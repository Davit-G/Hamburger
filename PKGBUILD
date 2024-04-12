# Maintainer: Will Hurley <will@hurleybirdjr.com>
pkgname="hamburger-git"
pkgver="r203.a4bd424"
pkgrel=1
pkgdesc="A distortion and dynamics plugin designed for sonic destruction and tasteful saturation."
arch=("x86_64")
url="https://aviaryaudio.com/plugins/hamburgerv2"
license=('GPLv3')
depends=()
makedepends=("git" "gcc")
# install=
# source=("$pkgname-$pkgver.tar.gz") - USE FOR BINARY
source=("Hamburger-Hurley::git://github.com/HurleybirdJr/Hamburger-Hurley.git")
sha256sums=("SKIP")

pkgver() {
  cd "$pkgname"
  printf "r%s.%s" "$(git rev-list --count HEAD)" "$(git rev-parse --short=7 HEAD)"
}

build() {
	cd "$pkgname"
	make
}

package() {
	cd "$pkgname"
	install -Dm755 ./Hamburger "$pkgdir/usr/bin/hamburger"
	install -Dm644 ./README.md "$pkgdir/usr/share/doc/$pkgname"
}
