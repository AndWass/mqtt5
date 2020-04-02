from conans import ConanFile

class Mqtt5(ConanFile):
    name = "mqtt5"
    version = "1.0.0"
    generators = "cmake"
    options = {"build_tests": [True, False]}
    default_options = {"build_tests": False}
    requires = ("span-lite/0.7.0",
        "boost/[>=1.71]@conan/stable",
        "p0443/0.0.4k"
        )

    def requirements(self):
        if self.options.build_tests:
            self.requires("doctest/2.3.5")