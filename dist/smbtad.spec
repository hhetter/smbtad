#
# spec file for package smbtad
#
# Copyright (c) 2010 SUSE LINUX Products GmbH, Nuernberg, Germany.
#
# All modifications and additions to the file contributed by third parties
# remain the property of their copyright owners, unless otherwise agreed
# upon. The license for this file, and modifications and additions to the
# file, is the same license as for the pristine package itself (unless the
# license for the pristine package is not an Open Source License, in which
# case the license is the MIT License). An "Open Source License" is a
# license that conforms to the Open Source Definition (Version 1.9)
# published by the Open Source Initiative.

# Please submit bugfixes or comments via http://bugs.opensuse.org/
#

# norootforbuild


Name:           smbtad
BuildRequires:  cmake libtalloc-devel
BuildRequires:  sqlite3-devel
Requires:	sqlite3 >= 3.7
BuildRecommends: libiniparser-devel
License:        GPLv3+
Group:          Productivity/Networking/Samba
Version:        1.2
Release:        1
Summary:        A collector of smbd share usage data
Url:            http://github.com/hhetter/smbtad
Source0:        %{name}-%{version}.tar.bz2
BuildRoot:      %{_tmppath}/%{name}-%{version}-build

%define INITDIR %{_sysconfdir}/init.d

%description
smbtad is the data receiver of the SMB Traffic Analyzer project.
With SMB Traffic Analyzer statistics about the data flow on a Samba network can be created.
Please see http://holger123.wordpress.com/smb-traffic-analyzer/ for more information. 

%prep
%setup -q

%build
if test ! -e "build"; then
  %{__mkdir} build
fi

pushd build
cmake \
  -DCMAKE_C_FLAGS:STRING="%{optflags}" \
  -DCMAKE_CXX_FLAGS:STRING="%{optflags}" \
  -DCMAKE_SKIP_RPATH=ON \
  -DCMAKE_BUILD_TYPE=RelWithDebInfo \
  -DCMAKE_INSTALL_PREFIX=%{_prefix} \
  -DSYSCONF_INSTALL_DIR=%{_sysconfdir} \
%if %{_lib} == lib64
  -DLIB_SUFFIX=64 \
%endif
  %{_builddir}/%{name}-%{version}
%__make %{?jobs:-j%jobs} VERBOSE=1
popd build

%install
pushd build
%if 0%{?suse_version}
%makeinstall
%else
make DESTDIR=%{buildroot} install
%endif
popd build
mkdir -p %{buildroot}/usr/sbin
ln -s /etc/init.d/smbtad %{buildroot}/usr/sbin/rcsmbtad

%clean
%__rm -rf %{buildroot}

%files
%defattr(-,root,root)
%{_bindir}/smbtad
%{_sbindir}/rcsmbtad
%attr(0754,root,root) %config %{INITDIR}/smbtad
%doc dist/smbtad.conf_example
%changelog
