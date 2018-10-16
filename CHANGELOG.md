# Changelog
All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](http://keepachangelog.com/en/1.0.0/)
and this project adheres to [Semantic Versioning](http://semver.org/spec/v2.0.0.html).

## [Unreleased]
- CMC, CID values are logged during SET
- Tests are added for factory-reset
- CMC set logic for factory-reset notification is changed
- Tests are added for replace table
- ReplaceTable crashes are fixed
- Removed CM agent dependency
- config json override issue in field is prevented
- Integrated WebPA Client Support in RDKB RPI and EMU 
- Updated README.md with pre-requisite 
- Added NULL checks in libpd.c
- abort from configure if thread support isn't found 
- Build errors while yocto integration are fixed
- Code is synced with CcspWebpaAdapter
- Integration tests for get values, set values, get attributes and set attributes are added
- Fixed set values crash for large parameter name/value
- Added setter functions for extern variables
- Added Fedora equivalent name for libexpat-dev.
- Added wrapper function for logging using cimplog library
- Added CcspWebpaAdapter code from gerrit repo
- Build fixes
    - openssl version mismatch 
    - log length >4MB
- External dependencies dbus & ccsp-commom-library are added

## [0.0.1] - 2017-06-15
### Added
- Initial creation

[Unreleased]: https://github.com/Comcast/parodus2ccsp/compare/953ff88c5ad415ca486fe06164e794efa8021c5d...HEAD
