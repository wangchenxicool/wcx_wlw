#!/bin/bash

INSTALL=install -o $(USER) -g $(USER)

VPN_BIN_PATH=/usr/sbin/
VPN_PPP_PATH=/etc/ppp/
VPN_PEERS_PATH=/etc/ppp/peers/

all:
	@echo "Please 'Make install!'"

install:
	@$(INSTALL) -m 755 -d $(VPN_BIN_PATH)
	@$(INSTALL) -m 755 ./pptp $(VPN_BIN_PATH)
	@$(INSTALL) -m 755 -d $(VPN_PPP_PATH)
	@$(INSTALL) -m 755 ./options.pptp $(VPN_PPP_PATH)
	@$(INSTALL) -m 755 ./chap-secrets $(VPN_PPP_PATH)
	@$(INSTALL) -m 755 -d $(VPN_PEERS_PATH)
	@$(INSTALL) -m 755 ./vpn $(VPN_PEERS_PATH)

