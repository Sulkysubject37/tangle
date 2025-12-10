class Tangle < Formula
  desc "Static PPI network analysis library with CLI and TUI"
  homepage "https://subconc.hashnode.dev/tangle"
  url "https://github.com/Sulkysubject37/tangle/archive/refs/tags/v0.1.0.tar.gz"
  sha256 "REPLACE_WITH_REAL_SHA256" # TODO: fill in after first release tarball
  license "MIT"

  head "https://github.com/Sulkysubject37/tangle.git", branch: "main"

  depends_on "cmake" => :build

  def install
    system "cmake", "-S", ".", "-B", "build",
           *std_cmake_args,
           "-DTANGLE_WITH_JSON=ON",
           "-DBUILD_TANGLE_CLI=ON",
           "-DBUILD_TANGLE_TUI=ON"
    system "cmake", "--build", "build"
    system "cmake", "--install", "build"
  end

  test do
    # Basic sanity check: CLI prints usage when run without subcommand
    output = shell_output("#{bin}/tangle 2>&1", 1)
    assert_match "Usage: tangle", output
  end
end
