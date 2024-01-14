source /etc/os-release

case "$ID" in
    "debian" | "linuxmint" | "ubuntu")
        export PKG_UPDATE="apt-get update"
        export PKG_INSTALL="apt-get install -y --user"
        ;;
    "fedora")
        export PKG_UPDATE="dnf update"
        export PKG_INSTALL="dnf install"
        ;;
    "arch")
        export PKG_UPDATE="pacman -Syu"
        export PKG_INSTALL="pacman -S"
        ;;
    "opensuse")
        export PKG_UPDATE="zypper refresh && zypper update"
        export PKG_INSTALL="zypper install"
        ;;
    *)
        echo "Unknown distro family"
        ;;
esac
