from pathlib import Path
import os
from conans import ConanFile, tools
from conan.tools.cmake import CMakeToolchain, CMake


def bool_to_cmake_definition(bool_value):
    if bool_value:
        return "ON"

    return "OFF"


# Conan docs
# - https://docs.conan.io/en/latest/developing_packages/package_dev_flow.html
# - https://docs.conan.io/en/latest/cheatsheet.html
# - https://docs.conan.io/en/latest/reference/conanfile.html
class IlluviumSimulationConan(ConanFile):
    name = "libIlluviumSimulation"
    description = "The coolest determinism library in the whole universe"
    license = "Copyright Illuvium DAO"
    url = "https://github.com/IlluviumGame/simulation"
    author = "Illuvium DAO"

    settings = "os", "compiler", "build_type", "arch"
    generators = "CMakeDeps"

    # NOTE: spdlog also ships with fmt, so the version spdlog ships must match the fmt version
    requires = "nlohmann_json/3.11.3", "spdlog/1.14.1", "fmt/10.2.1"

    # See recipes:
    # - https://conan.io/center/gtest?tab=recipe
    # - https://conan.io/center/spdlog?tab=recipe&
    # - https://conan.io/center/fmt?tab=recipe
    options = {
        "enable_profiling": [True, False],
        "enable_testing": [True, False],
        "enable_unity_build": [True, False],
        "enable_ue": [True, False],
        "enable_cli": [True, False],
        "enable_visualization": [True, False],
        "warnings_as_errors": [True, False],
        "ue_path": "ANY",
    }
    default_options = {
        # Our options
        "enable_profiling": False,
        "enable_testing": True,
        "enable_unity_build": False,
        "enable_ue": False,
        "enable_cli": True,
        "enable_visualization": False,
        "warnings_as_errors": True,
        "ue_path": "~/UnrealEngine/",
        # Third party libraries options
        "gtest:shared": False,
        "gtest:build_gmock": True,
        "fmt:header_only": True,
        "spdlog:shared": False,
        "spdlog:header_only": True,
        "spdlog:no_exceptions": True,
        "easy_profiler:shared": False,
    }

    @property
    def packaged_root_dir(self):
        return "SimulationLibrary"

    @property
    def packaged_libs_dir(self):
        return f"{self.packaged_root_dir}/Libraries"

    @property
    def packaged_includes_dir(self):
        return f"{self.packaged_root_dir}/Includes"

    @property
    def packaged_binaries_dir(self):
        return "Binaries"

    @property
    def simulation_lib_dir_name(self):
        return "simulation_lib"

    @property
    def simulation_lib_src_dir(self):
        return f"{self.simulation_lib_dir_name}/src"

    @property
    def simulation_lib_build_dir(self):
        return f"{self.build_folder}/{self.simulation_lib_dir_name}"

    @property
    def compiled_lib_name(self):
        suffix = "UNKNOWN"
        if self.settings.os == "Windows":
            suffix = "Win64.lib"
        elif self.settings.os == "Linux":
            if self.settings.arch == "armv8":
                suffix = "Linux-arm64.a"
            else:
                suffix = "Linux-x64.a"
        elif self.settings.os == "Macos":
            suffix = "macOS.a"
        elif self.settings.os == "Android":
            suffix = "Android.a"

        return f"{self.name}-{suffix}"

    def __init__(self, output, runner, display_name="", user=None, channel=None):
        super().__init__(output, runner, display_name, user, channel)
        self.output.info("__init__()")

    def export_sources(self):
        self.output.info("export_sources()")

        for glob_expression in ["*.h*", "*.cc"]:
            self.copy(glob_expression, src=self.simulation_lib_src_dir, dst="Simulation")

        self.copy("CMakeLists.txt")
        self.copy("cmake/*")

    def requirements(self):
        self.output.info("requirements()")

        if self.options.enable_profiling:
            self.requires("easy_profiler/2.1.0")
        if self.options.enable_testing:
            self.requires("gtest/1.11.0")
        if self.options.enable_cli:
            self.requires("lyra/1.6.1")
        if self.options.enable_visualization:
            self.requires("raylib/4.0.0")

    def configure(self):
        self.output.info("configure()")

        print("-- CONFIGURE -- ")
        print(f"version = {self.version}")

    def set_version(self):
        # Set the Version to the git commit
        # https://docs.conan.io/en/latest/reference/conanfile/methods.html#set-name-set-version
        git = tools.Git(folder=self.recipe_folder)

        # Try default to get the git branch
        git_branch = git.get_branch()
        if "HEAD" in git_branch:
            git_branch = ""

        # Use the version from the environment
        # NOTE: Useful inside jenkins
        if "GIT_BRANCH" in os.environ:
            git_branch = os.environ["GIT_BRANCH"]

        if git_branch:
            git_branch = git_branch + "_"

            # Some branches can have the / separator
            git_branch = git_branch.replace("/", "").replace("release", "")

        # NOTE: Command matches version_git_internal.h.in
        MAX_LEN_GIT_BRANCH = 40
        MAX_LEN_GIT_COMMIT = 10
        self.version = f"{git_branch[:MAX_LEN_GIT_BRANCH]}{git.get_commit()[:MAX_LEN_GIT_COMMIT]}"

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

    def generate(self):
        tc = CMakeToolchain(self)
        tc.user_presets_path = False
        tc.variables["ENABLE_TESTING"] = bool_to_cmake_definition(self.options.enable_testing)
        tc.variables["ENABLE_PROFILING"] = bool_to_cmake_definition(self.options.enable_profiling)
        tc.variables["ENABLE_UNITY_BUILD"] = bool_to_cmake_definition(self.options.enable_unity_build)
        tc.variables["WARNINGS_AS_ERRORS"] = bool_to_cmake_definition(self.options.warnings_as_errors)
        tc.variables["ENABLE_UE"] = bool_to_cmake_definition(self.options.enable_ue)
        tc.variables["UE_PATH"] = self.options.ue_path
        tc.variables["ENABLE_CLI"] = bool_to_cmake_definition(self.options.enable_cli)
        tc.variables["ENABLE_VISUALIZATION"] = bool_to_cmake_definition(self.options.enable_visualization)
        tc.variables["CONAN_TARGET_ARCH"] = self.settings.arch
        tc.generate()

    def package_info(self):
        self.output.info("package_info()")

        # https://docs.conan.io/en/latest/reference/conanfile/methods.html#package-info

        # The libs to link against
        self.cpp_info.libs = [self.compiled_lib_name]

        # Ordered list of include paths
        self.cpp_info.includedirs = [self.packaged_includes_dir]

        # Directories where libraries can be found
        self.cpp_info.libdirs = [self.packaged_libs_dir]

    def package(self):
        self.output.info("package()")

        # Copy headers
        dst_headers_dir = f"{self.packaged_includes_dir}/Simulation"
        print(f"package: Copying own headers, from {self.simulation_lib_src_dir} to {dst_headers_dir}")
        self.copy("*.h", dst=dst_headers_dir, src=self.simulation_lib_src_dir)
        self.copy("*.h", dst=dst_headers_dir, src=f"{self.build_folder}/simulation_lib")

        # Copy JSON library header
        nlohmann_json_include_path = str(self.deps_cpp_info["nlohmann_json"].include_paths[0])
        print(f"package: Copying nlohmann_json headers, src = {nlohmann_json_include_path}, dst = {dst_headers_dir}")
        self.copy("*.hpp", dst=dst_headers_dir, src=nlohmann_json_include_path)

        # Copy easy_profiler headers
        if self.options.enable_profiling:
            easy_profiler_include_path = str(self.deps_cpp_info["easy_profiler"].include_paths[0])
            print(
                f"package: Copying easy_profiler headers, src = {easy_profiler_include_path}, dst = {dst_headers_dir}"
            )
            self.copy("*.h", dst=dst_headers_dir, src=easy_profiler_include_path)

        fmt_include_path = str(self.deps_cpp_info["fmt"].include_paths[0])

        # Copy format library that ships with nlohmann_json
        print(f"package: Copying fmt headers, src = {fmt_include_path}, dst = {dst_headers_dir}")
        self.copy("*.h", dst=dst_headers_dir, src=fmt_include_path)

        # Copy simulation libraries
        dst_libs_dir = self.packaged_libs_dir
        if self.settings.os == "Windows":
            self.copy("*libIlluviumSimulation*.lib", dst=dst_libs_dir, keep_path=False)
            self.copy("*libIlluviumSimulation*.pdb", dst=dst_libs_dir, keep_path=False)
            # self.copy("*.dll", dst=dst_libs_dir, keep_path=False)
        else:
            self.copy("*.dylib*", dst=dst_libs_dir, keep_path=False)
            self.copy("*.so", dst=dst_libs_dir, keep_path=False)
            self.copy("*.a", dst=dst_libs_dir, keep_path=False)

        # Copy easy_profiler library
        if self.options.enable_profiling:
            easy_profiler_lib_path = str(self.deps_cpp_info["easy_profiler"].lib_paths[0])
            self.copy("*.lib", dst=dst_libs_dir, src=easy_profiler_lib_path)
            self.copy("*.a", dst=dst_libs_dir, src=easy_profiler_lib_path)

        # Copy the simulation cli binary
        if self.options.enable_cli:
            self.copy("simulation-cli*", dst=self.packaged_binaries_dir, src="bin/", keep_path=False)

    @classmethod
    def patch_file(cls, filename, replacements):
        if not replacements:
            return

        patch_data = cls.read_file(filename)

        # Perform each of the replacements in the supplied dictionary
        for t in replacements:
            old, new = t
            patch_data = patch_data.replace(old, new)

        cls.write_file(filename, patch_data)

    @classmethod
    def read_file(cls, filename):
        with open(file=str(filename), mode="r", encoding="utf-8") as f:
            return f.read()

    @classmethod
    def write_file(cls, filename, data):
        with open(file=str(filename), mode="r", encoding="utf-8") as f:
            f.write(data)

    @classmethod
    def path_relative_to_absolute(cls, path_raw):
        return Path(str(path_raw)).resolve()
