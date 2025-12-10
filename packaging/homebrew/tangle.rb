class Tangle < Formula
  desc "Static PPI network analysis library with CLI and TUI"
  homepage "https://subconc.hashnode.dev/tangle"
  url "https://github.com/Sulkysubject37/tangle/archive/refs/tags/v0.1.1.tar.gz"
  sha256 "9cb8b6646094d61675f466c110144983099316f514bc950ddc2dc6180296a2db"
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
