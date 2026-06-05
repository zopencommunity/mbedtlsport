/*
 * hardware_entropy.c
 *
 * Platform entropy source for Mbed TLS on z/OS UNIX System Services (USS).
 *
 * --------------------------------------------------------------------------
 * Why Mbed TLS calls this function
 * --------------------------------------------------------------------------
 * Mbed TLS gathers seed material through an entropy context
 * (mbedtls_entropy_context), which mixes together one or more registered
 * "entropy sources" and feeds the result to the DRBG (CTR_DRBG / HMAC_DRBG)
 * that ultimately produces the random bytes used for key generation, nonces,
 * IVs, and so on.
 *
 * On platforms where Mbed TLS does not recognize the operating system's own
 * entropy interface, the integrator supplies the source. This is done by
 * enabling MBEDTLS_ENTROPY_HARDWARE_ALT in the Mbed TLS configuration and
 * providing a function with this exact name and signature:
 *
 *     int mbedtls_hardware_poll(void *data, unsigned char *output,
 *                               size_t len, size_t *olen);
 *
 * When MBEDTLS_ENTROPY_HARDWARE_ALT is defined, mbedtls_entropy_init()
 * automatically registers mbedtls_hardware_poll() as a STRONG entropy source.
 * If no strong source is registered, mbedtls_entropy_func() refuses to produce
 * output (MBEDTLS_ERR_ENTROPY_NO_STRONG_SOURCE) rather than emit weak random
 * data. This function is therefore the single point on which the security of
 * all Mbed TLS randomness on this platform depends.
 *
 * Contract:
 *   - Write up to len bytes of entropy into output.
 *   - Store the number of bytes actually produced in *olen.
 *   - Return 0 on success, or MBEDTLS_ERR_ENTROPY_SOURCE_FAILED on failure.
 *   - The entropy collector may call this repeatedly until it has gathered a
 *     full block, so each call must return fresh entropy.
 *
 * --------------------------------------------------------------------------
 * Entropy source on z/OS USS
 * --------------------------------------------------------------------------
 * Per IBM documentation, the character special files /dev/random and
 * /dev/urandom (major 4, minor 2) provide cryptographically secure random
 * output generated from the available cryptographic hardware (ICSF). The
 * foundation of this random number generation is a time-variant input with a
 * very low probability of recycling.
 *
 *   https://www.ibm.com/docs/en/zos/2.4.0?topic=files-random-number
 *
 * We read directly from /dev/urandom, which is non-blocking and is a
 * cryptographically strong source on this platform. No additional "mixing"
 * (such as a store-clock value) is performed: XOR-ing a predictable, monotonic
 * value into already-strong output adds no entropy, and would add nothing
 * useful in the case where the strong read failed.
 *
 * --------------------------------------------------------------------------
 * Failure policy: fail closed
 * --------------------------------------------------------------------------
 * If /dev/urandom cannot be opened or fully read, this function returns an
 * error and produces no output. It deliberately does NOT fall back to any
 * weaker source. Returning success with predictable data (a "fail open")
 * would silently downgrade every key and nonce derived afterwards, which is
 * the most dangerous failure mode an RNG can have. Failing closed lets Mbed
 * TLS's NO_STRONG_SOURCE guard stop the operation instead.
 *
 * The most likely cause of failure here is that ICSF/CSF is not active or the
 * device is not configured. That condition is best detected once at process
 * startup rather than on the first cryptographic operation.
 */

#define _POSIX_SOURCE

#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include "mbedtls/entropy.h"

#ifdef __MVS__

/*
 * read() may return fewer bytes than requested, or be interrupted by a
 * signal. Loop until the whole buffer is filled. EINTR is retried; a return
 * of 0 (EOF) from /dev/urandom is abnormal and is treated as a failure.
 */
static int read_full (int fd, unsigned char *output_buffer, size_t number_of_bytes_requested)
{
   size_t total_retrieved = 0;
   while (total_retrieved < number_of_bytes_requested)
   {
      ssize_t bytes_read = read (fd, output_buffer + total_retrieved, number_of_bytes_requested - total_retrieved);
      if (bytes_read <= 0)
      {
         if (bytes_read < 0 && errno == EINTR)
         {
            continue;
         }
         return -1;
      }
      total_retrieved += (size_t) bytes_read;
   }
   return 0;
}

/*
 * Strong entropy source for Mbed TLS. Registered automatically as a strong
 * source when MBEDTLS_ENTROPY_HARDWARE_ALT is defined. See the file header for
 * the full rationale and contract.
 */
int mbedtls_hardware_poll (void *data, unsigned char *output, size_t length, size_t *output_length)
{
   (void) data;
   *output_length = 0;

   int fd = open ("/dev/urandom", O_RDONLY);
   if (fd < 0)
   {
      return MBEDTLS_ERR_ENTROPY_SOURCE_FAILED;
   }

   if (read_full (fd, output, length) != 0)
   {
      close (fd);
      return MBEDTLS_ERR_ENTROPY_SOURCE_FAILED;
   }

   close (fd);

   *output_length = length;
   return 0;
}

#endif /* __MVS__ */

