Name: t-search-kingso-libutil
Version:0.0.1
Release: %(echo $RELEASE)%{?dist}
Summary: Kingso-libutil
URL: %{_svn_path}@%{_svn_revision}
Group: search/newse
License: Commercial

BuildRequires: alog-devel >= 1.0.6 
BuildRequires: search-mxml >= 2.2.2 , pool-devel >= 1.0.2  

%description
%{_svn_path}
%{_svn_revision}
Kingso-libutil
%prep
%build
cd $OLDPWD/../
sh autogen.sh
./configure --prefix=${RPM_BUILD_ROOT}/home/admin/release/kingso-libutil-%{_version}-%{_release} --with-realrunpath=/home/admin/newse
make -j8
%install
cd $OLDPWD/../
make install
%post

%files
%defattr(-,admin,admin)
/home/admin/release/

%changelog


