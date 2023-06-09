#	$NetBSD: t_route.sh,v 1.15 2022/09/20 02:25:07 knakahara Exp $
#
# Copyright (c) 2016 Internet Initiative Japan Inc.
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
# ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
# TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
# PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
# BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.
#

# non_subnet_gateway
SOCK_CLIENT=unix://commsock1
SOCK_GW=unix://commsock2
BUS=bus1

# command_get
SOCKSRC=unix://commsock1
SOCKFWD=unix://commsock2
SOCKDST=unix://commsock3
IP4SRC=10.0.1.2
IP4SRCGW=10.0.1.1
IP4DSTGW=10.0.2.1
IP4DST=10.0.2.2
IP4DST_BCAST=10.0.2.255
IP6SRC=fc00:0:0:1::2
IP6SRCGW=fc00:0:0:1::1
IP6DSTGW=fc00:0:0:2::1
IP6DST=fc00:0:0:2::2
BUS_SRCGW=bus1
BUS_DSTGW=bus2
# command_add
SOCKHOST=unix://commsock1

DEBUG=${DEBUG:-false}
TIMEOUT=1
PING_OPTS="-n -c 1 -w $TIMEOUT"

atf_test_case route_non_subnet_gateway cleanup
route_non_subnet_gateway_head()
{

	atf_set "descr" "tests of a gateway not on the local subnet"
	atf_set "require.progs" "rump_server"
}

route_non_subnet_gateway_body()
{

	rump_server_start $SOCK_CLIENT
	rump_server_start $SOCK_GW

	export RUMP_SERVER=${SOCK_GW}
	rump_server_add_iface $SOCK_GW shmif0 $BUS
	atf_check -s exit:0 rump.ifconfig shmif0 192.168.0.1
	atf_check -s exit:0 rump.ifconfig shmif0 up

	# The gateway knows the client
	atf_check -s exit:0 -o match:'add net 10.0.0.1: gateway shmif0' \
	    rump.route add -net 10.0.0.1/32 -link -cloning -iface shmif0

	$DEBUG && rump.netstat -nr -f inet

	export RUMP_SERVER=${SOCK_CLIENT}
	rump_server_add_iface $SOCK_CLIENT shmif0 $BUS
	atf_check -s exit:0 rump.ifconfig shmif0 10.0.0.1/32
	atf_check -s exit:0 rump.ifconfig shmif0 up
	atf_check -s exit:0 rump.ifconfig -w 10

	$DEBUG && rump.netstat -nr -f inet

	# Don't know a route to the gateway yet
	atf_check -s not-exit:0 -o match:'100.0% packet loss' \
	    -e match:'No route to host' rump.ping $PING_OPTS 192.168.0.1

	# Teach a route to the gateway
	atf_check -s exit:0 -o match:'add net 192.168.0.1: gateway shmif0' \
	    rump.route add -net 192.168.0.1/32 -link -cloning -iface shmif0
	atf_check -s exit:0 -o match:'add net default: gateway 192.168.0.1' \
	    rump.route add default -ifa 10.0.0.1 192.168.0.1

	$DEBUG && rump.netstat -nr -f inet

	# Be reachable to the gateway
	atf_check -s exit:0 -o ignore rump.ping $PING_OPTS 192.168.0.1

	rump_server_destroy_ifaces
}

route_non_subnet_gateway_cleanup()
{

	$DEBUG && dump
	cleanup
}

atf_test_case route_command_get cleanup
atf_test_case route_command_get6 cleanup
route_command_get_head()
{

	atf_set "descr" "tests of route get command"
	atf_set "require.progs" "rump_server"
}

route_command_get6_head()
{

	atf_set "descr" "tests of route get command (IPv6)"
	atf_set "require.progs" "rump_server"
}

setup_endpoint()
{
	local sock=${1}
	local addr=${2}
	local bus=${3}
	local mode=${4}
	local gw=${5}

	export RUMP_SERVER=${sock}
	rump_server_add_iface $sock shmif0 $bus
	if [ $mode = "ipv6" ]; then
		atf_check -s exit:0 rump.ifconfig shmif0 inet6 ${addr}
		atf_check -s exit:0 -o ignore rump.route add -inet6 default ${gw}
	else
		atf_check -s exit:0 rump.ifconfig shmif0 inet ${addr} netmask 0xffffff00
		atf_check -s exit:0 -o ignore rump.route add default ${gw}
	fi
	atf_check -s exit:0 rump.ifconfig shmif0 up
	atf_check -s exit:0 rump.ifconfig -w 10

	if $DEBUG; then
		rump.ifconfig shmif0
		rump.netstat -nr
	fi
}

setup_forwarder()
{
	mode=${1}

	rump_server_add_iface $SOCKFWD shmif0 $BUS_SRCGW
	rump_server_add_iface $SOCKFWD shmif1 $BUS_DSTGW

	export RUMP_SERVER=$SOCKFWD
	if [ $mode = "ipv6" ]; then
		atf_check -s exit:0 rump.ifconfig shmif0 inet6 ${IP6SRCGW}
		atf_check -s exit:0 rump.ifconfig shmif1 inet6 ${IP6DSTGW}
	else
		atf_check -s exit:0 rump.ifconfig shmif0 inet ${IP4SRCGW} netmask 0xffffff00
		atf_check -s exit:0 rump.ifconfig shmif1 inet ${IP4DSTGW} netmask 0xffffff00
	fi

	atf_check -s exit:0 rump.ifconfig shmif0 up
	atf_check -s exit:0 rump.ifconfig shmif1 up
	atf_check -s exit:0 rump.ifconfig -w 10

	if $DEBUG; then
		rump.netstat -nr
		if [ $mode = "ipv6" ]; then
			rump.sysctl net.inet6.ip6.forwarding
		else
			rump.sysctl net.inet.ip.forwarding
		fi
	fi
}

setup_forwarding()
{
	export RUMP_SERVER=$SOCKFWD
	atf_check -s exit:0 -o ignore rump.sysctl -w net.inet.ip.forwarding=1
}

setup_forwarding6()
{
	export RUMP_SERVER=$SOCKFWD
	atf_check -s exit:0 -o ignore rump.sysctl -w net.inet6.ip6.forwarding=1
}

setup()
{

	rump_server_start $SOCKSRC
	rump_server_start $SOCKFWD
	rump_server_start $SOCKDST

	setup_endpoint $SOCKSRC $IP4SRC $BUS_SRCGW ipv4 $IP4SRCGW
	setup_endpoint $SOCKDST $IP4DST $BUS_DSTGW ipv4 $IP4DSTGW
	setup_forwarder ipv4
}

setup6()
{

	rump_server_start $SOCKSRC netinet6
	rump_server_start $SOCKFWD netinet6
	rump_server_start $SOCKDST netinet6

	setup_endpoint $SOCKSRC $IP6SRC $BUS_SRCGW ipv6 $IP6SRCGW
	setup_endpoint $SOCKDST $IP6DST $BUS_DSTGW ipv6 $IP6DSTGW
	setup_forwarder ipv6
}

test_route_get()
{

	export RUMP_SERVER=$SOCKSRC
	$DEBUG && rump.netstat -nr -f inet
	$DEBUG && rump.arp -n -a

	# Make sure an ARP cache to the gateway doesn't exist
	rump.arp -d $IP4SRCGW

	# Local
	cat >./expect <<-EOF
   route to: 10.0.1.2
destination: 10.0.1.2
 local addr: 10.0.1.2
  interface: lo0
      flags: 0x40045<UP,HOST,DONE,LOCAL>
 recvpipe  sendpipe  ssthresh  rtt,msec    rttvar  hopcount      mtu     expire
	EOF
	rump.route -n get $IP4SRC > ./output
	$DEBUG && cat ./expect ./output
	# XXX: omit the last line because expire is unstable on rump kernel.
	sed -i '$d' ./output
	atf_check -s exit:0 diff ./expect ./output

	# Neighbor
	cat >./expect <<-EOF
   route to: 10.0.1.1
destination: 10.0.1.0
       mask: 255.255.255.0
 local addr: 10.0.1.2
  interface: shmif0
      flags: 0x141<UP,DONE,CONNECTED>
 recvpipe  sendpipe  ssthresh  rtt,msec    rttvar  hopcount      mtu     expire
	EOF
	rump.route -n get $IP4SRCGW > ./output
	$DEBUG && cat ./expect ./output
	sed -i '$d' ./output
	atf_check -s exit:0 diff ./expect ./output

	# Remote host
	cat >./expect <<-EOF
   route to: 10.0.2.2
destination: default
       mask: default
    gateway: 10.0.1.1
 local addr: 10.0.1.2
  interface: shmif0
      flags: 0x843<UP,GATEWAY,DONE,STATIC>
 recvpipe  sendpipe  ssthresh  rtt,msec    rttvar  hopcount      mtu     expire
	EOF
	rump.route -n get $IP4DST > ./output
	$DEBUG && cat ./expect ./output
	sed -i '$d' ./output
	atf_check -s exit:0 diff ./expect ./output

	# Create a ARP cache
	atf_check -s exit:0 -o ignore rump.ping -q -n -w $TIMEOUT -c 1 $IP4SRCGW

	# Neighbor with a cache (no different from w/o cache)
	cat >./expect <<-EOF
   route to: 10.0.1.1
destination: 10.0.1.0
       mask: 255.255.255.0
 local addr: 10.0.1.2
  interface: shmif0
      flags: 0x141<UP,DONE,CONNECTED>
 recvpipe  sendpipe  ssthresh  rtt,msec    rttvar  hopcount      mtu     expire
	EOF
	rump.route -n get $IP4SRCGW > ./output
	$DEBUG && cat ./expect ./output
	sed -i '$d' ./output
	atf_check -s exit:0 diff ./expect ./output
}

test_route_get6()
{

	export RUMP_SERVER=$SOCKSRC
	$DEBUG && rump.netstat -nr -f inet
	$DEBUG && rump.ndp -n -a

	# Make sure an ARP cache to the gateway doesn't exist
	rump.ndp -d $IP6SRCGW

	# Local
	cat >./expect <<-EOF
   route to: fc00:0:0:1::2
destination: fc00:0:0:1::2
 local addr: fc00:0:0:1::2
  interface: lo0
      flags: 0x40045<UP,HOST,DONE,LOCAL>
 recvpipe  sendpipe  ssthresh  rtt,msec    rttvar  hopcount      mtu     expire
	EOF
	rump.route -n get -inet6 $IP6SRC > ./output
	$DEBUG && cat ./expect ./output
	sed -i '$d' ./output
	atf_check -s exit:0 diff ./expect ./output

	# Neighbor
	cat >./expect <<-EOF
   route to: fc00:0:0:1::1
destination: fc00:0:0:1::
       mask: ffff:ffff:ffff:ffff::
 local addr: fc00:0:0:1::2
  interface: shmif0
      flags: 0x141<UP,DONE,CONNECTED>
 recvpipe  sendpipe  ssthresh  rtt,msec    rttvar  hopcount      mtu     expire
	EOF
	rump.route -n get -inet6 $IP6SRCGW > ./output
	$DEBUG && cat ./expect ./output
	sed -i '$d' ./output
	atf_check -s exit:0 diff ./expect ./output

	# Remote host
	cat >./expect <<-EOF
   route to: fc00:0:0:2::2
destination: ::
       mask: default
    gateway: fc00:0:0:1::1
 local addr: fc00:0:0:1::2
  interface: shmif0
      flags: 0x843<UP,GATEWAY,DONE,STATIC>
 recvpipe  sendpipe  ssthresh  rtt,msec    rttvar  hopcount      mtu     expire
	EOF
	rump.route -n get -inet6 $IP6DST > ./output
	$DEBUG && cat ./expect ./output
	sed -i '$d' ./output
	atf_check -s exit:0 diff ./expect ./output

	# Create a NDP cache
	atf_check -s exit:0 -o ignore rump.ping6 -n -c 1 -X $TIMEOUT $IP6SRCGW

	# Neighbor with a cache (no different from w/o cache)
	cat >./expect <<-EOF
   route to: fc00:0:0:1::1
destination: fc00:0:0:1::
       mask: ffff:ffff:ffff:ffff::
 local addr: fc00:0:0:1::2
  interface: shmif0
      flags: 0x141<UP,DONE,CONNECTED>
 recvpipe  sendpipe  ssthresh  rtt,msec    rttvar  hopcount      mtu     expire
	EOF
	rump.route -n get -inet6 $IP6SRCGW > ./output
	$DEBUG && cat ./expect ./output
	sed -i '$d' ./output
	atf_check -s exit:0 diff ./expect ./output
}

route_command_get_body()
{

	setup
	setup_forwarding
	test_route_get
	rump_server_destroy_ifaces
}

route_command_get6_body()
{

	setup6
	setup_forwarding6
	test_route_get6
	rump_server_destroy_ifaces
}

route_command_get_cleanup()
{

	$DEBUG && dump
	cleanup
}

route_command_get6_cleanup()
{

	$DEBUG && dump
	cleanup
}

atf_test_case route_default_reject cleanup
route_default_reject_head()
{

	atf_set "descr" "tests for making a default route reject"
	atf_set "require.progs" "rump_server"
}

route_default_reject_body()
{

	rump_server_start $SOCKSRC netinet6
	rump_server_add_iface $SOCKSRC shmif0 $BUS_SRCGW

	export RUMP_SERVER=$SOCKSRC

	# From /etc/rc.d/network
	atf_check -s exit:0 -o match:'add net ::0.0.0.0: gateway ::1' \
	    rump.route add -inet6 ::0.0.0.0 -prefixlen 104 ::1 -reject

	atf_check -s exit:0 rump.ifconfig shmif0 inet6 $IP6SRC/64 up
	$DEBUG && rump.netstat -nr -f inet6
	atf_check -s exit:0 -o match:"add net default: gateway $IP6SRCGW" \
	    rump.route add -inet6 default $IP6SRCGW
	$DEBUG && rump.netstat -nr -f inet6
	$DEBUG && rump.route -n get -inet6 default
	atf_check -s exit:0 -o match:'change net default' \
	    rump.route change -inet6 default -reject
	$DEBUG && rump.netstat -nr -f inet6

	rump_server_destroy_ifaces
}

route_default_reject_cleanup()
{

	$DEBUG && dump
	cleanup
}

atf_test_case route_command_add cleanup
route_command_add_head()
{

	atf_set "descr" "tests of route add command"
	atf_set "require.progs" "rump_server"
}

route_command_add_body()
{

	rump_server_start $SOCKHOST

	export RUMP_SERVER=${SOCKHOST}
	rump_server_add_iface $SOCKHOST shmif0 $BUS
	atf_check -s exit:0 rump.ifconfig shmif0 10.0.0.1/24

	# Accept the route whose gateway is in a subnet of interface address
	atf_check -s exit:0 -o ignore rump.route add \
	    -net 10.0.1.0/24 10.0.0.2
	atf_check -s exit:0 -o ignore rump.route add \
	    -host 10.0.2.1 10.0.0.3

	# Accept the route whose gateway is an interface
	atf_check -s exit:0 -o ignore rump.route add \
	    -net 10.0.3.0/24 -connected -link -iface shmif0

	# Accept the route whose gateway is reachable and not RTF_GATEWAY
	atf_check -s exit:0 -o ignore rump.route add \
	    -net 10.0.4.0/24 10.0.3.1

	# Don't accept the route whose destination is reachable and
	# gateway is unreachable
	atf_check -s not-exit:0 -o ignore -e match:'unreachable' rump.route add \
	    -net 10.0.1.0/26 10.0.5.1

	# Don't accept the route whose gateway is reachable and RTF_GATEWAY
	atf_check -s not-exit:0 -o ignore -e ignore rump.route add \
	    -net 10.0.6.0/24 10.0.1.1

	rump_server_destroy_ifaces
}

route_command_add_cleanup()
{

	$DEBUG && dump
	cleanup
}

atf_test_case route_command_add6 cleanup
route_command_add6_head()
{

	atf_set "descr" "tests of route add command (IPv6)"
	atf_set "require.progs" "rump_server"
}

route_command_add6_body()
{

	rump_server_start $SOCKHOST netinet6

	export RUMP_SERVER=${SOCKHOST}
	rump_server_add_iface $SOCKHOST shmif0 $BUS
	atf_check -s exit:0 rump.ifconfig shmif0 inet6 fc00::1/64

	# Accept the route whose gateway is in a subnet of interface address
	atf_check -s exit:0 -o ignore rump.route add -inet6\
	    -net fc00:1::0/64 fc00::2
	atf_check -s exit:0 -o ignore rump.route add -inet6\
	    -host fc00:2::1 fc00::3

	# Accept the route whose gateway is an interface
	atf_check -s exit:0 -o ignore rump.route add -inet6\
	    -net fc00:3::0/64 -connected -link -iface shmif0

	# Accept the route whose gateway is reachable and not RTF_GATEWAY
	atf_check -s exit:0 -o ignore rump.route add -inet6\
	    -net fc00:4::0/64 fc00:3::1

	# Don't accept the route whose destination is reachable and
	# gateway is unreachable
	atf_check -s not-exit:0 -o ignore -e match:'unreachable' rump.route add \
	    -inet6 -net fc00::4/128 fc00:5::1

	# Don't accept the route whose gateway is reachable and RTF_GATEWAY
	atf_check -s not-exit:0 -o ignore -e match:'unreachable' rump.route add \
	    -inet6 -net fc00:6::0/64 fc00:1::1

	rump_server_destroy_ifaces
}

route_command_add6_cleanup()
{

	$DEBUG && dump
	cleanup
}

test_route_address_removal()
{

	rump_server_start $SOCKHOST netinet6

	export RUMP_SERVER=${SOCKHOST}
	rump_server_add_iface $SOCKHOST shmif0 $BUS

	#
	# 1. test auto removal of a route that depends a removing address
	#
	atf_check -s exit:0 rump.ifconfig shmif0 $af $addr1/$prefix
	atf_check -s exit:0 -o match:"add net $alt_net(/$prefix)?: gateway $addrgw" \
	    rump.route -n add -$af -net $alt_net/$prefix $addrgw
	$DEBUG && rump.netstat -nr -f $af
	atf_check -s exit:0 rump.ifconfig shmif0 $af $addr1 delete
	$DEBUG && rump.netstat -nr -f $af

	# The route should be deleted on the address removal
	atf_check -s not-exit:0 -e match:"writing to routing socket: not in table" \
	    rump.route -n get -$af $alt_addr

	#
	# 2. test auto update of a route that depends a removing address where
	#    there is another address with the same prefix sharing a connected
	#    route
	#
	atf_check -s exit:0 rump.ifconfig shmif0 $af $addr1/$prefix
	atf_check -s exit:0 rump.ifconfig shmif0 $af $addr2/$prefix alias
	atf_check -s exit:0 -o match:"add net $alt_net(/$prefix)?: gateway $addrgw" \
	    rump.route -n add -$af -net $alt_net/$prefix $addrgw
	$DEBUG && rump.netstat -nr -f $af

	atf_check -s exit:0 -o match:"local addr: $addr1" \
	    rump.route -n get -$af $addrgw
	atf_check -s exit:0 -o match:"local addr: $addr1" \
	    rump.route -n get -$af $alt_addr

	atf_check -s exit:0 rump.ifconfig shmif0 $af $addr1 delete
	$DEBUG && rump.netstat -nr -f $af

	# local addr (rt_ifa) of the connected route should be changed
	# on the address removal
	atf_check -s exit:0 -o match:"local addr: $addr2" \
	    rump.route -n get -$af $addrgw
	# local addr (rt_ifa) of the related route should be changed
	# on the address removal too
	atf_check -s exit:0 -o match:"local addr: $addr2" \
	    rump.route -n get -$af $alt_addr

	# cleanup
	atf_check -s exit:0 rump.ifconfig shmif0 $af $addr2 delete

	#
	# 3. test auto update of a route that depends a removing address where
	#    there is another address with a different (short) prefix
	#
	atf_check -s exit:0 rump.ifconfig shmif0 $af $addr1/$prefix
	atf_check -s exit:0 rump.ifconfig shmif0 $af $addr2/$prefix_short alias
	atf_check -s exit:0 -o match:"add net $alt_net(/$prefix)?: gateway $addrgw" \
	    rump.route -n add -$af -net $alt_net/$prefix $addrgw
	$DEBUG && rump.netstat -nr -f $af

	atf_check -s exit:0 -o match:"local addr: $addr1" \
	    rump.route -n get -$af $addrgw
	atf_check -s exit:0 -o match:"local addr: $addr1" \
	    rump.route -n get -$af $alt_addr

	atf_check -s exit:0 rump.ifconfig shmif0 $af $addr1 delete
	$DEBUG && rump.netstat -nr -f $af

	# local addr (rt_ifa) of the connected route should be changed
	# on the address removal
	atf_check -s exit:0 -o match:"local addr: $addr2" \
	    rump.route -n get -$af $addrgw
	if [ $af = inet ]; then
		# local addr (rt_ifa) of the related route should be changed
		# on the address removal too
		atf_check -s exit:0 -o match:"local addr: $addr2" \
		    rump.route -n get -$af $alt_addr
	else
		# For IPv6, each address has its own connected route so the
		# address removal just results in a removal of the related route
		atf_check -s not-exit:0 \
		    -e match:"writing to routing socket: not in table" \
		    rump.route -n get -$af $alt_addr
	fi

	rump_server_destroy_ifaces
}

atf_test_case route_address_removal cleanup
route_address_removal_head()
{

	atf_set "descr" "tests of auto removal/update of routes on address removal (IPv4)"
	atf_set "require.progs" "rump_server"
}

route_address_removal_body()
{
	local addr1=10.0.0.1
	local addr2=10.0.0.2
	local addrgw=10.0.0.3
	local prefix=24
	local prefix_short=16
	local alt_net=10.0.1.0
	local alt_addr=10.0.1.1
	local af=inet

	test_route_address_removal
}

route_address_removal_cleanup()
{

	$DEBUG && dump
	cleanup
}

atf_test_case route_address_removal6 cleanup
route_address_removal6_head()
{

	atf_set "descr" "tests of auto removal/update of routes on address removal (IPv6)"
	atf_set "require.progs" "rump_server"
}

route_address_removal6_body()
{
	local addr1=fd00::1
	local addr2=fd00::2
	local addrgw=fd00::3
	local prefix=64
	local prefix_short=32
	local alt_net=fd00:1::0
	local alt_addr=fd00:1::1
	local af=inet6

	test_route_address_removal
}

route_address_removal6_cleanup()
{

	$DEBUG && dump
	cleanup
}


atf_init_test_cases()
{

	atf_add_test_case route_non_subnet_gateway
	atf_add_test_case route_command_get
	atf_add_test_case route_command_get6
	atf_add_test_case route_default_reject
	atf_add_test_case route_command_add
	atf_add_test_case route_command_add6
	atf_add_test_case route_address_removal
	atf_add_test_case route_address_removal6
}
