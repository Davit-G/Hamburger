FROM archlinux

RUN pacman -Sy --noconfirm base-devel git

RUN useradd builduser -m \
  && passwd -d builduser \
  && cd /home/builduser \
  && git clone "https://github.com/HurleybirdJr/Hamburger-Hurley.git" hamburger \
  && chown builduser -R hamburger \
  && (printf 'builduser ALL=(ALL) ALL\n' | tee -a /etc/sudoers) \
  && sudo -u builduser bash -c 'cd ~/hamburger && makepkg -si --noconfirm' \
  && pacman -Rns $(pacman -Qtdq)

WORKDIR /home/builduser

CMD ["bash"]