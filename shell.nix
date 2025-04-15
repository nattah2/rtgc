{ pkgs ? import <nixpkgs> {} }:

pkgs.mkShell {
  name = "realtime-gc-dev";

  buildInputs = with pkgs; [
    gcc
    gnumake
    gdb
    valgrind
    glibc
    time
    gnuplot
    gdb
    openjdk17   # or openjdk21 if you prefer the latest with ZGC
  ];

  shellHook = ''
    echo "=== <Trailblazer> Real-Time Garbage Environment ==="
    echo "Available tools: gcc, gdb, valgrind, perf, gnuplot, Java (ZGC-capable)"
    export JAVA_HOME=${pkgs.openjdk17}
  '';
}
