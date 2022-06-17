#!/usr/bin/env python

# Copyright (C) 2015-2019, Wazuh Inc.
# Created by Wazuh, Inc. <info@wazuh.com>.
# This program is a free software; you can redistribute it and/or modify it under the terms of GPLv2

from setuptools import setup, find_packages

# To install the library, run the following
#
# python setup.py install
#
# prerequisite: setuptools
# http://pypi.python.org/pypi/setuptools

setup(
    name='api',
    version='4.2.4',
    description="Hids API",
    author_email="hello@wazuh.com",
    author="Hids",
    url="https://www.zhiannet.com",
    keywords=["Hids API"],
    install_requires=[],
    packages=find_packages(exclude=["*.test", "*.test.*", "test.*", "test"]),
    package_data={'': ['spec/spec.yaml']},
    include_package_data=True,
    zip_safe=False,
    license='GPLv2',
    long_description="""\
    The Hids API is an open source RESTful API that allows for interaction with the hids manager from a web browser, command line tool like cURL or any script or program that can make web requests. The Hids Kibana app relies on this heavily and hids’s goal is to accommodate complete remote management of the Hids infrastructure via the Hids Kibana app. Use the API to easily perform everyday actions like adding an agent, restarting the manager(s) or agent(s) or looking up syscheck details.
    """
)
