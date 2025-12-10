class Tangle < Formula
  desc "Static PPI network analysis library with CLI and TUI"
  homepage "https://subconc.hashnode.dev/tangle"
  url "https://github.com/Sulkysubject37/tangle/archive/refs/tags/v0.1.0.tar.gz"
  sha256 "d4741170a1ea8ed33495d2f25e132b5a525e1af6acf092f6690cd08af95ba514"
  license "MIT"

  head "https://github.com/Sulkysubject37/tangle.git", branch: "main"

  depends_on "cmake" => :build
  depends_on "ftxui"

  def install
    system "cmake", "-S", ".", "-B", "build",
           *std_cmake_args,
           "-DTANGLE_WITH_JSON=OFF",
           "-DBUILD_TANGLE_CLI=ON",
           "-DBUILD_TANGLE_TUI=ON",
           "-DTANGLE_USE_SYSTEM_FTXUI=ON"
    system "cmake", "--build", "build"
    system "cmake", "--install", "build"
  end

  test do
    # Basic sanity check: CLI prints usage when run without subcommand
    output = shell_output("#{bin}/tangle 2>&1", 1)
    assert_match "Usage: tangle", output
  end
end
