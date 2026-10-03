#!/usr/bin/env bash
# Instala y deja en PATH las dependencias para Boids (Demo 2).
# Uso:
#   chmod +x scripts/setup_linux.sh
#   ./scripts/setup_linux.sh
#   source ~/.bashrc   # si el script añadió rutas al PATH

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "${SCRIPT_DIR}/../.." && pwd)"
MARKER_DIR="${HOME}/.local/share/boids-demo2"
MARKER_FILE="${MARKER_DIR}/path.env"
LOCAL_BIN="${HOME}/.local/bin"

mkdir -p "${LOCAL_BIN}" "${MARKER_DIR}"

info()  { printf '==> %s\n' "$*"; }
warn()  { printf '!!  %s\n' "$*" >&2; }
have()  { command -v "$1" >/dev/null 2>&1; }

detect_pkg_manager() {
  if have apt-get; then echo apt
  elif have dnf; then echo dnf
  elif have pacman; then echo pacman
  elif have zypper; then echo zypper
  else echo none
  fi
}

sudo_run() {
  if [[ "${EUID}" -eq 0 ]]; then
    "$@"
  elif have sudo; then
    sudo "$@"
  else
    warn "Se necesitan privilegios de administrador para: $*"
    return 1
  fi
}

install_packages() {
  local mgr
  mgr="$(detect_pkg_manager)"
  case "${mgr}" in
    apt)
      info "Actualizando índices APT..."
      sudo_run apt-get update -y
      info "Instalando build-essential, cmake, git, pkg-config..."
      sudo_run apt-get install -y \
        build-essential cmake git pkg-config \
        libgl1-mesa-dev libx11-dev libxrandr-dev libxi-dev \
        libxcursor-dev libxinerama-dev libwayland-dev libxkbcommon-dev
      # Raylib opcional del sistema; CMake también puede descargarlo.
      if apt-cache show libraylib-dev >/dev/null 2>&1; then
        sudo_run apt-get install -y libraylib-dev || true
      fi
      ;;
    dnf)
      info "Instalando paquetes con DNF..."
      sudo_run dnf install -y \
        gcc-c++ cmake git pkgconf-pkg-config \
        mesa-libGL-devel libX11-devel libXrandr-devel libXi-devel \
        libXcursor-devel libXinerama-devel
      sudo_run dnf install -y raylib-devel || true
      ;;
    pacman)
      info "Instalando paquetes con pacman..."
      sudo_run pacman -Sy --noconfirm \
        base-devel cmake git pkgconf \
        mesa libx11 libxrandr libxi libxcursor libxinerama
      sudo_run pacman -S --noconfirm raylib || true
      ;;
    zypper)
      info "Instalando paquetes con zypper..."
      sudo_run zypper --non-interactive install \
        gcc-c++ cmake git pkg-config \
        Mesa-libGL-devel libX11-devel libXrandr-devel libXi-devel \
        libXcursor-devel libXinerama-devel
      ;;
    *)
      warn "No se detectó un gestor de paquetes soportado."
      warn "Instale manualmente: g++/clang++, cmake >= 3.16, git."
      ;;
  esac
}

ensure_cmake() {
  if have cmake; then
    local ver
    ver="$(cmake --version | head -n1)"
    info "CMake encontrado: ${ver}"
    return 0
  fi
  info "CMake no está en PATH; instalando..."
  install_packages
  if ! have cmake; then
    warn "CMake sigue sin encontrarse después de la instalación."
    return 1
  fi
}

ensure_compiler() {
  if have g++ || have clang++; then
    if have g++; then info "Compilador: $(g++ --version | head -n1)"
    else info "Compilador: $(clang++ --version | head -n1)"
    fi
    return 0
  fi
  info "No hay compilador C++; instalando toolchain..."
  install_packages
  if ! have g++ && ! have clang++; then
    warn "No se encontró g++ ni clang++."
    return 1
  fi
}

ensure_git() {
  if have git; then
    info "Git encontrado: $(git --version)"
    return 0
  fi
  install_packages
}

append_path_persist() {
  local path_entry="$1"
  case ":${PATH}:" in
    *":${path_entry}:"*) ;;
    *) export PATH="${path_entry}:${PATH}" ;;
  esac

  local line="export PATH=\"${path_entry}:\$PATH\""
  printf '%s\n' "${line}" > "${MARKER_FILE}"

  for rc in "${HOME}/.bashrc" "${HOME}/.zshrc" "${HOME}/.profile"; do
    if [[ -f "${rc}" ]] || [[ "${rc}" == "${HOME}/.bashrc" ]]; then
      touch "${rc}"
      if ! grep -Fq "${MARKER_FILE}" "${rc}" 2>/dev/null; then
        {
          echo ""
          echo "# Boids Demo 2 toolchain"
          echo "[ -f \"${MARKER_FILE}\" ] && . \"${MARKER_FILE}\""
        } >> "${rc}"
        info "PATH persistente añadido en ${rc}"
      fi
    fi
  done
}

verify() {
  info "Verificación final"
  have cmake && cmake --version | head -n1 || warn "cmake ausente"
  if have g++; then g++ --version | head -n1
  elif have clang++; then clang++ --version | head -n1
  else warn "compilador C++ ausente"
  fi
  have git && git --version || warn "git ausente"
  info "Raíz del repositorio: ${PROJECT_ROOT}"
  info "Siguiente paso: ./proyect_1-Multithreading/scripts/run_demo2.sh"
}

main() {
  info "Setup Linux — Boids Demo 2"
  append_path_persist "${LOCAL_BIN}"
  ensure_git
  ensure_compiler
  ensure_cmake
  # Si el gestor no instaló todo, un segundo intento ayuda en distros mínimas.
  if ! have cmake || { ! have g++ && ! have clang++; }; then
    install_packages
  fi
  verify
  info "Listo. Si abrió una terminal nueva, el PATH ya debería incluir ${LOCAL_BIN}."
}

main "$@"
