#!/usr/bin/env python3
"""
Setup script for robotstxt Python bindings.

To install:
    pip install .

For development:
    pip install -e .
"""

from setuptools import setup, find_packages

setup(
    name="robotstxt",
    version="1.0.0",
    description="Python bindings for Google's robots.txt parser library",
    long_description=open("README.md").read() if __import__("os").path.exists("README.md") else "",
    long_description_content_type="text/markdown",
    author="Google LLC",
    license="Apache-2.0",
    url="https://github.com/google/robotstxt",
    packages=find_packages(),
    python_requires=">=3.7",
    classifiers=[
        "Development Status :: 5 - Production/Stable",
        "Intended Audience :: Developers",
        "License :: OSI Approved :: Apache Software License",
        "Programming Language :: Python :: 3",
        "Programming Language :: Python :: 3.7",
        "Programming Language :: Python :: 3.8",
        "Programming Language :: Python :: 3.9",
        "Programming Language :: Python :: 3.10",
        "Programming Language :: Python :: 3.11",
        "Programming Language :: Python :: 3.12",
        "Topic :: Internet :: WWW/HTTP",
        "Topic :: Software Development :: Libraries",
    ],
    keywords="robots.txt, robots, crawler, parser, web",
)
