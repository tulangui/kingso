%define _topdir %(pwd)/rpmbuild
%define _tmppath %{_topdir}/tmp
%define buildroot %{_tmppath}/%{name}-root
%define source    SOURCE/kingso_opensource_%{version}.tar.gz
%define _unpackaged_files_terminate_build 0

Summary: kingso_opensource
Name: kingso_opensource
Version: 0.8.6
Release: 1
Source: %{source}
License: GPL
Group: kingso
BuildRoot: %{buildroot}
Prefix:%{_prefix}

%description
This package provides kingso rpm for opensource
%prep
%setup -q
%build
sh ./run.sh
%install
rm -rf %{buildroot}
mkdir -p %{buildroot}
cp -r %{_topdir}/BUILD/kingso_opensource-0.8.6/build/dst/ %{buildroot}/%{_prefix}
%post

%clean
rm -rf %{buildroot}

%files
%defattr(-,root,root)
%config %{_prefix}/etc/
%{_bindir}
%{_prefix}/lib
%{_prefix}/logs
%{_prefix}/example
%{_prefix}/detail_plugin

