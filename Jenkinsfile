import groovy.json.JsonSlurperClassic

node {
    def ARTIFACTS_PATH = 'build/repository/org/muhkuh/lua/papa-schlumpf-flex/*'
    def strBuilds = env.JENKINS_SELECT_BUILDS
    def atBuilds = new JsonSlurperClassic().parseText(strBuilds)

    atBuilds.each { atEntry ->
        stage("${atEntry[0]} ${atEntry[1]} ${atEntry[2]}"){
            docker.image("${atEntry[3]}").inside('-u root') {
                /* Clean before the build. */
                sh 'rm -rf .[^.] .??* *'

                checkout([$class: 'GitSCM',
                    branches: [[name: env.GIT_BRANCH_SPECIFIER]],
                    doGenerateSubmoduleConfigurations: false,
                    extensions: [
                        [$class: 'SubmoduleOption',
                            disableSubmodules: false,
                            recursiveSubmodules: true,
                            reference: '',
                            trackingSubmodules: false
                        ]
                    ],
                    submoduleCfg: [],
                    userRemoteConfigs: [[url: 'https://github.com/muhkuh-sys/org.muhkuh.lua-papa-schlumpf-flex.git']]
                ])

                /* Build the project. */
                sh "python3 build_artifact.py ${atEntry[0]} ${atEntry[1]} ${atEntry[2]}"

                /* Archive all artifacts. */
                archiveArtifacts artifacts: "${ARTIFACTS_PATH0}/*.tar.xz,${ARTIFACTS_PATH0}/*.xml,${ARTIFACTS_PATH0}/*.hash,${ARTIFACTS_PATH0}/*.pom"

                /* Clean up after the build. */
                sh 'rm -rf .[^.] .??* *'
            }
        }
    }
}
