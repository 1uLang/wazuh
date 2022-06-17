#!/usr/bin/env python

# Copyright (C) 2015-2020, Wazuh Inc.
# Created by Wazuh, Inc. <info@wazuh.com>.
# This program is free software; you can redistribute it and/or modify it under the terms of GPLv2

import json
# Install the package locally: python setup.py install
# Install the package dev: python setup.py develop
import os
from datetime import datetime

from setuptools import setup, find_packages
from setuptools.command.install import install


class InstallCommand(install):
    user_options = install.user_options + [
        ('Hids-version=', None, 'Hids Version'),
        ('install-type=', None, 'Installation type: server, local, hybrid')
    ]

    def initialize_options(self):
        install.initialize_options(self)
        self.wazuh_version = None
        self.install_type = None

    def finalize_options(self):
        install.finalize_options(self)

    def run(self):
        here = os.path.abspath(os.path.dirname(__file__))
        with open(os.path.join(here, 'hids', 'core', 'hids.json'), 'w') as f:
            json.dump({'install_type': self.install_type,
                       'hids_version': self.wazuh_version,
                       'installation_date': datetime.utcnow().strftime('%a %b %d %H:%M:%S UTC %Y')
                       }, f)
        # install.run(self)  # OR: install.do_egg_install(self)
        install.do_egg_install(self)


setup(name='hids',
      version='4.2.4',
      description='Hids control with Python',
      url='www.zhiannet.com',
      author='Hids',
      author_email='hello@wazuh.com',
      license='GPLv2',
      packages=find_packages(exclude=["*.tests", "*.tests.*", "tests.*", "tests"]),
      package_data={'hids': ['core/hids.json', 'core/cluster/cluster.json', 'rbac/default/*.yaml']},
      include_package_data=True,
      install_requires=[],
      zip_safe=False,
      cmdclass={
          'install': InstallCommand
      }
      )
