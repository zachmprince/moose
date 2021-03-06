### Transitional step for pre-existing users

For those of you who have previously installed the moose-environment package, you should remove it. +If you are a first time MOOSE user, please skip down to [Install Conda](conda.md#installconda).+ Removal of the moose-envirnment package, is only needed to be performed once.

- Using Conda, it is no longer neccessary to have /opt/moose present on your machine. Depending on the type of machine you have, please do the following:

  | Operating System | Command |
  | :- | -: |
  | CentOS | `sudo yum remove moose-environment` |
  | Fedora | `sudo dnf remove moose-environment` |
  | OpenSUSE | `sudo zypper remove moose-environment` |
  | Debian (Ubuntu, Mint) | `sudo dpkg -r moose-environment` |
  | Macintosh | `sudo rm -rf /opt/moose` |

  !alert warning title= sudo is dangerous
  Be especially carful with the above commands! Verify +twice+ that what you have entered in your terminal is what the instructions are asking you to do.
