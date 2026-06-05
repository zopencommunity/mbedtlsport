node('linux') {
  stage ('Poll') {
    checkout([
      $class: 'GitSCM', branches: [[name: '*/main']], extensions: [],
      userRemoteConfigs: [[url: 'https://github.com/zopencommunity/mbedtls3.6.6port.git']]])
  }
  stage('Build') {
    build job: 'Port-Pipeline', parameters: [
      string(name: 'PORT_GITHUB_REPO', value: 'https://github.com/zopencommunity/mbedtls3.6.6port.git'),
      string(name: 'PORT_DESCRIPTION', value: 'Mbed TLS is a C library that implements X.509 certificate manipulation and the TLS and DTLS protocols.'),
      string(name: 'BUILD_LINE', value: 'DEV')
    ]
  }
}
