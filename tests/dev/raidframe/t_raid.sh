#! /usr/bin/atf-sh
#	$NetBSD: t_raid.sh,v 1.16 2022/11/30 17:49:09 martin Exp $
#
# Copyright (c) 2010 The NetBSD Foundation, Inc.
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

rawpart=$( set -- a b c d e f g h i j k l m n o p q r s t u v w x y z;
	shift $( sysctl -n kern.rawpartition ); printf %s "$1" )
rawraid=/dev/rraid0${rawpart}
raidserver="rump_server -lrumpvfs -lrumpdev -lrumpdev_disk -lrumpdev_raidframe"

makecfg()
{
	level=${1}
	ncol=${2}

	printf "START array\n${ncol} 0\nSTART disks\n" > raid.conf
	diskn=0
	while [ ${ncol} -gt ${diskn} ] ; do
		echo "/disk${diskn}" >> raid.conf
		diskn=$((diskn+1))
	done

	printf "START layout\n32 1 1 ${level}\nSTART queue\nfifo 100\n" \
	    >> raid.conf
}

makecfg_old()
{
	level=${1}
	ncol=${2}

	printf "START array\n1 ${ncol} 0\nSTART disks\n" > raid.conf
	diskn=0
	while [ ${ncol} -gt ${diskn} ] ; do
		echo "/disk${diskn}" >> raid.conf
		diskn=$((diskn+1))
	done

	printf "START layout\n32 1 1 ${level}\nSTART queue\nfifo 100\n" \
	    >> raid.conf
}

atf_test_case smalldisk cleanup
smalldisk_head()
{
	atf_set "descr" "Checks the raidframe works on small disks " \
	    "(PR kern/44239)"
	atf_set "require.progs" "rump_server"
}

smalldisk_body_backend()
{
	export RUMP_SERVER=unix://sock
	atf_check -s exit:0 ${raidserver}			\
	    -d key=/disk0,hostpath=disk0.img,size=1m		\
	    -d key=/disk1,hostpath=disk1.img,size=1m		\
	    ${RUMP_SERVER}

	atf_check -s exit:0 rump.raidctl -C raid.conf raid0
}

smalldisk_body()
{
	makecfg 1 2
	smalldisk_body_backend
}

smalldisk_cleanup()
{
	export RUMP_SERVER=unix://sock
	rump.halt
}

# The old configuration test case uses the smalldisk backend
atf_test_case old_numrows_config cleanup
old_numrows_config_head()
{
	atf_set "descr" "Checks the old numRows configuration works"
	atf_set "require.progs" "rump_server"
}

old_numrows_config_body()
{
	makecfg_old 1 2
	smalldisk_body_backend
}

old_numrows_config_cleanup()
{
	export RUMP_SERVER=unix://sock
	rump.halt
}

export RAID_MEDIASIZE=4m

atf_test_case raid1_compfail cleanup
raid1_compfail_head()
{
	atf_set "descr" "Checks that RAID1 works after component failure"
	atf_set "require.progs" "rump_server"
}

raid1_compfail_body()
{
	makecfg 1 2
	export RUMP_SERVER=unix://sock
	atf_check -s exit:0 ${raidserver}				\
	    -d key=/disk0,hostpath=disk0.img,size=${RAID_MEDIASIZE}	\
	    -d key=/disk1,hostpath=disk1.img,size=${RAID_MEDIASIZE}	\
	    ${RUMP_SERVER}

	atf_check -s exit:0 rump.raidctl -C raid.conf raid0
	atf_check -s exit:0 rump.raidctl -I 12345 raid0
	atf_check -s exit:0 -o ignore rump.raidctl -iv raid0

	# put some data there
	atf_check -s exit:0 -e ignore \
	    dd if=$(atf_get_srcdir)/t_raid of=testfile count=4
	atf_check -s exit:0 -e ignore -x \
	    "dd if=testfile | rump.dd of=${rawraid} conv=sync"

	# restart server with failed component
	rump.halt
	rm disk1.img # FAIL
	atf_check -s exit:0 ${raidserver}				\
	    -d key=/disk0,hostpath=disk0.img,size=${RAID_MEDIASIZE}	\
	    -d key=/disk1,hostpath=disk1.img,size=${RAID_MEDIASIZE}	\
	    ${RUMP_SERVER}

	atf_check -s exit:0 rump.raidctl -c raid.conf raid0

	# check if we get what we wrote
	atf_check -s exit:0 -o file:testfile -e ignore \
	    rump.dd if=${rawraid} count=4
}

raid1_compfail_cleanup()
{
	export RUMP_SERVER=unix://sock
	rump.halt
}



atf_test_case raid1_comp0fail cleanup
raid1_comp0fail_head()
{
	atf_set "descr" "Checks configuring RAID1 after component 0 fails" \
		"(PR kern/44251)"
	atf_set "require.progs" "rump_server"
}

raid1_comp0fail_body()
{
	makecfg 1 2
	export RUMP_SERVER=unix://sock
	atf_check -s exit:0 ${raidserver}				\
	    -d key=/disk0,hostpath=disk0.img,size=${RAID_MEDIASIZE}	\
	    -d key=/disk1,hostpath=disk1.img,size=${RAID_MEDIASIZE}	\
	    ${RUMP_SERVER}

	atf_check -s exit:0 rump.raidctl -C raid.conf raid0
	atf_check -s exit:0 rump.raidctl -I 12345 raid0
	atf_check -s exit:0 -o ignore rump.raidctl -iv raid0

	# restart server with failed component
	rump.halt
	rm disk0.img # FAIL
	atf_check -s exit:0 ${raidserver} 				\
	    -d key=/disk0,hostpath=disk0.img,size=${RAID_MEDIASIZE}	\
	    -d key=/disk1,hostpath=disk1.img,size=${RAID_MEDIASIZE}	\
	    ${RUMP_SERVER}

	atf_check -s exit:0 rump.raidctl -c raid.conf raid0
}

raid1_comp0fail_cleanup()
{
	export RUMP_SERVER=unix://sock
	rump.halt
}

atf_test_case raid1_normal cleanup
raid1_normal_head()
{
	atf_set "descr" "Checks that RAID1 -c configurations work " \
		"in the normal case"
	atf_set "require.progs" "rump_server"
}

raid1_normal_body()
{
	makecfg 1 2
	export RUMP_SERVER=unix://sock
        atf_check -s exit:0 ${raidserver}                               \
            -d key=/disk0,hostpath=disk0.img,size=${RAID_MEDIASIZE}     \
            -d key=/disk1,hostpath=disk1.img,size=${RAID_MEDIASIZE}     \
            ${RUMP_SERVER}

        atf_check -s exit:0 rump.raidctl -C raid.conf raid0
        atf_check -s exit:0 rump.raidctl -I 12345 raid0
        atf_check -s exit:0 -o ignore rump.raidctl -iv raid0

        # put some data there
        atf_check -s exit:0 -e ignore \
            dd if=$(atf_get_srcdir)/t_raid of=testfile count=4
        atf_check -s exit:0 -e ignore -x \
            "dd if=testfile | rump.dd of=${rawraid} conv=sync"

        # restart server, disks remain normal 
        rump.halt

        atf_check -s exit:0 ${raidserver}                               \
            -d key=/disk0,hostpath=disk0.img,size=${RAID_MEDIASIZE}     \
            -d key=/disk1,hostpath=disk1.img,size=${RAID_MEDIASIZE}     \
            ${RUMP_SERVER}

        atf_check -s exit:0 rump.raidctl -c raid.conf raid0

        # check if we get what we wrote
        atf_check -s exit:0 -o file:testfile -e ignore \
            rump.dd if=${rawraid} count=4

}

raid1_normal_cleanup()
{       
        export RUMP_SERVER=unix://sock
        rump.halt
}


atf_test_case raid5_compfail cleanup
raid5_compfail_head()
{
	atf_set "descr" "Checks that RAID5 works after component failure"
	atf_set "require.progs" "rump_server"
}

raid5_compfail_body()
{
	makecfg 5 3
	export RUMP_SERVER=unix://sock
	atf_check -s exit:0 ${raidserver}				\
	    -d key=/disk0,hostpath=disk0.img,size=${RAID_MEDIASIZE}	\
	    -d key=/disk1,hostpath=disk1.img,size=${RAID_MEDIASIZE}	\
	    -d key=/disk2,hostpath=disk2.img,size=${RAID_MEDIASIZE}	\
	    ${RUMP_SERVER}

	atf_check -s exit:0 rump.raidctl -C raid.conf raid0
	atf_check -s exit:0 rump.raidctl -I 12345 raid0
	atf_check -s exit:0 -o ignore rump.raidctl -iv raid0

	# put some data there
	atf_check -s exit:0 -e ignore \
	    dd if=$(atf_get_srcdir)/t_raid of=testfile count=4
	atf_check -s exit:0 -e ignore -x \
	    "dd if=testfile | rump.dd of=${rawraid} conv=sync"

	# restart server with failed component
	rump.halt
	rm disk2.img # FAIL
	atf_check -s exit:0 ${raidserver}				\
	    -d key=/disk0,hostpath=disk0.img,size=${RAID_MEDIASIZE}	\
	    -d key=/disk1,hostpath=disk1.img,size=${RAID_MEDIASIZE}	\
	    -d key=/disk2,hostpath=disk2.img,size=${RAID_MEDIASIZE}	\
	    ${RUMP_SERVER}

	atf_check -s exit:0 rump.raidctl -c raid.conf raid0

	# check if we get what we wrote
	atf_check -s exit:0 -o file:testfile -e ignore \
	    rump.dd if=${rawraid} count=4
}

raid5_compfail_cleanup()
{
	export RUMP_SERVER=unix://sock
	rump.halt
}

atf_test_case raid5_normal cleanup
raid5_normal_head()
{
        atf_set "descr" "Checks that RAID5 works after normal shutdown " \
		"and 'raidctl -c' startup"
	atf_set "require.progs" "rump_server"
}

raid5_normal_body()
{
        makecfg 5 3
        export RUMP_SERVER=unix://sock
        atf_check -s exit:0 ${raidserver}                               \
            -d key=/disk0,hostpath=disk0.img,size=${RAID_MEDIASIZE}     \
            -d key=/disk1,hostpath=disk1.img,size=${RAID_MEDIASIZE}     \
            -d key=/disk2,hostpath=disk2.img,size=${RAID_MEDIASIZE}     \
            ${RUMP_SERVER}

        atf_check -s exit:0 rump.raidctl -C raid.conf raid0
        atf_check -s exit:0 rump.raidctl -I 12345 raid0
        atf_check -s exit:0 -o ignore rump.raidctl -iv raid0

        # put some data there
        atf_check -s exit:0 -e ignore \
            dd if=$(atf_get_srcdir)/t_raid of=testfile count=4
        atf_check -s exit:0 -e ignore -x \
            "dd if=testfile | rump.dd of=${rawraid} conv=sync"

        # restart server after normal shutdown
        rump.halt

        atf_check -s exit:0 ${raidserver}                               \
            -d key=/disk0,hostpath=disk0.img,size=${RAID_MEDIASIZE}     \
            -d key=/disk1,hostpath=disk1.img,size=${RAID_MEDIASIZE}     \
            -d key=/disk2,hostpath=disk2.img,size=${RAID_MEDIASIZE}     \
            ${RUMP_SERVER}

        atf_check -s exit:0 rump.raidctl -c raid.conf raid0

        # check if we get what we wrote
        atf_check -s exit:0 -o file:testfile -e ignore \
            rump.dd if=${rawraid} count=4
}

raid5_normal_cleanup()
{
        export RUMP_SERVER=unix://sock
        rump.halt
}

atf_init_test_cases()
{
	atf_add_test_case smalldisk
	atf_add_test_case old_numrows_config
	atf_add_test_case raid1_normal
	atf_add_test_case raid1_comp0fail
	atf_add_test_case raid1_compfail
	atf_add_test_case raid5_normal
	atf_add_test_case raid5_compfail
}
