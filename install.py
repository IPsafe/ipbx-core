#!/usr/bin/python
# coding: utf-8
# Copyright (C) 2010 Guilherme Bessa Rezende
#
# Author: Guilherme Rezende <guilhermebr@gmail.com>
#

import os
import sys
import platform


class Install(object):
    ''' Generic class '''
    def __init__(self):
        self.env = {}
        self.dependences = []

    def setup(self):
        self.env['processor'] = os.uname()[4] #os.popen("uname -p").read()
        self.env['architecture'] = platform.architecture()[0]
        self.env['path'] = os.path.realpath(os.path.dirname(__file__))
        self.env['system'] = platform.system()
        self.env['dist'] = platform.dist()[0]
  
    def install(self):
        ''' Implement in your custom class '''
        pass

    def update_centos(self):
        print "Updating CentOS"
        if os.system("yum -y --enablerepo=centosplus --enablerepo=contrib update") == 0:
            print "System Updated"
        else:
            print "Command error."

    def update_debian(self):
        pass

    def install_dependences(self):
        if self.env['system'] == 'Linux':
            if self.env['dist'] == 'redhat':
                command = 'yum -y install'
            elif self.env['dist'] == 'debian':
                command = 'apt-get install'
        else:
            print "This is not a Linux. =("
            return 
        deps = ''
        for dependence in self.dependences:
            deps += dependence + " "
        os.system("%s %s" % (command, deps))

#custom classes

class IPBX(Install):  
    def setup(self):

        super(IPBX, self).setup()
        self.env['dir_log'] = '/var/log/ipbx/'
        self.env['site_url'] = 'http://downloads.asterisk.org/pub/telephony/'
        self.env['libpri_version'] = '1.4.13'
        self.env['dahdi_version'] = '2.6.1+2.6.1'
        self.env['asterisk_version'] = '1.8.19.0'

        self.dependences = ['tar', 'mc', 'vim-enhanced', 'gcc', 'gcc-c++', 'ncurses*', 'bison', 
                             'flex', 'openssl', 'openssl-devel', 'gnutls-devel', 'zlib-devel', 
                             'ghostscript', 'make', 'subversion', 'wget', 'sox*', 'chkconfig', 
                             'vixie-cron', 'which', 'logrotate', 'postfix', 'lynx', 'gzip', 'bc', 
                             'postgresql84', 'postgresql84-server', 'postgresql84-devel', 'curl*', 
                             'libxml2', 'libxml2-devel', 'libstdc*', 'libpng*', 'libogg*', 'libvorb*',
                             'httpd', 'lame', 'ngrep', 'kernel-devel', 'kernel-xen-devel', 
                            ]

    def install(self):

        os.system('mkdir -p %(path)s/packages; mkdir -p %(dir_log)s' % self.env)
        self.update_centos()
        self.install_dependences()
        self.install_rpmforge()
        #config_database()
        #intall_webserver()
        self.install_dahdi()
        self.install_libpri()
        self.install_asterisk()
        #install_codecs()
        #instal_ipbx()

    def install_rpmforge(self):
        os.system("wget -P packages http://packages.sw.be/rpmforge-release/rpmforge-release-0.5.2-2.el5.rf.%(processor)s.rpm" % self.env)
        os.system("rpm --import http://apt.sw.be/RPM-GPG-KEY.dag.txt")
        os.system("rpm -i %(path)s/packages/rpmforge-release-0.5.2-2.el5.rf.%(processor)s.rpm" % self.env)

    def install_libpri(self):
        os.system("wget -P packages %(site_url)s/libpri/releases/libpri-%(libpri_version)s.tar.gz" % self.env)
        os.system("cd %(path)s/packages/; tar zxvf libpri-%(libpri_version)s.tar.gz" % self.env)
        os.system("cd %(path)s/packages/libpri-%(libpri_version)s; make; make install" % self.env)
   
    def install_dahdi(self):
        os.system("wget -P packages %(site_url)s/dahdi-linux-complete/releases/dahdi-linux-complete-%(dahdi_version)s.tar.gz" % self.env)
        os.system("cd %(path)s/packages/; tar zxvf %(path)s/packages/dahdi-linux-complete-%(dahdi_version)s.tar.gz" % self.env)
        os.system("cd %(path)s/packages/dahdi-linux-complete-%(dahdi_version)s; make; make install; make config" % self.env)

    def install_asterisk(self):
        os.system("wget -P packages %(site_url)s/asterisk/releases/asterisk-%(asterisk_version)s.tar.gz" % self.env)
        os.system("cd %(path)s/packages/; tar zxvf %(path)s/packages/asterisk-%(asterisk_version)s.tar.gz" % self.env)
        os.system("cd %(path)s/packages/asterisk-%(asterisk_version)s; ./configure; make; make install" % self.env)


new = IPBX()
new.setup()
new.install()