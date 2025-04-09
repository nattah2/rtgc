{ pkgs ? import <nixpkgs> {} }:

with pkgs;

mkShell {
  buildInputs = [
    libstdcxx5
    jdk17
  ];
}
