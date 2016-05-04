#faketelnetd

I wrote this in 2008 as a honeypot for a linux firewall running on a Nokia IPSO firewall. In 2016 I imported this to GitHub from the Google Code archive. While I doubt anybody, including me, will ever find this to be of use I enjoy having rediscovered this codebase if only for the sake of nostalgia.

## Installation

To build run:
make -f Makefile.production

Or, if you want a debug enabled build run:
make -f Makefile.debug

Then copy faketelnetd to /usr/local/bin/ or something like that:
cp ./faketelnetd /usr/local/bin/

Then copy the default config file into /etc:
cp ./faketelnetd.conf /etc

Now tweak your config file and start the daemon with:
/usr/local/bin/faketelnetd
