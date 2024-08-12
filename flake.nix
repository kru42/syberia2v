{
  description = "Half-Life 2 port for the PSVita";

  inputs = {
    vitasdk.url = "github:sleirsgoevy/vitasdk.nix";
    flake-utils.url = "github:numtide/flake-utils";
  };

  outputs = {
    self,
    nixpkgs,
    vitasdk,
    flake-utils,
  }:
    flake-utils.lib.eachDefaultSystem (system: let
      pkgs = import nixpkgs {system = system;};
    in {
      devShells.default = pkgs.mkShell rec {
        nativeBuildInputs = [pkgs.pkg-config];

        packages = [
          vitasdk.vitasdk
          vitasdk.vitaGL
        ];

        shellHook = ''
          echo "syberia2v dev environment initialized."
        '';
      };

    #   packages.default = pkgs.stdenv.mkDerivation {
    #     pname = "carrotway";
    #     version = "0.1";
    #
    #     src = ./.;
    #
    #     # packages = with pkgs; [
    #     # ];
    #
    #     buildPhase = ''
    #       make
    #     '';
    #
    #     installPhase = ''
    #       mkdir -p $out/bin
    #       cp carrotway $out/bin/carrot
    #     '';
    #
    #     meta = with pkgs.lib; {
    #       description = "a carrot-like wayland launcher";
    #       license = license.mit;
    #       platforms = platforms.linux;
    #       maintainers = with maintainers; [kru];
    #     };
    #   };
    # });
}
