{pkgs ? import <nixpkgs> {}}:

with pkgs;
let
  libunity = callPackage /etc/nixos/pkgs/libunity {};
in
stdenv.mkDerivation rec {
  name = "simple_tcpip";
  src = ./.;
  buildInputs = [ pkg-config  meson ninja libunity ];
  postBuild = "meson test";
}
