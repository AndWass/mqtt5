from conans import ConanFile

class Mqtt5(ConanFile):
    name = "mqtt5"
    version = "1.0.0"
    generators = "cmake"
    options = {
        "build_tests": [True, False],
        "ssl": [True, False]
        }
    default_options = {
        "build_tests": False,
        "ssl": True
        }
    requires = ("span-lite/0.7.0",
        "boost/[>=1.72]",
        "p0443/0.0.10",
        "SML/latest"
        )

    def requirements(self):
        if self.options.build_tests:
            self.requires("doctest/2.3.5")
        if self.options.ssl:
            self.requires("openssl/1.1.1f")