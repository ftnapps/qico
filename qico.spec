Summary:	qico, an ftn-compatible mailer
Name:		qico
Version:	0.53si
Release:	1
Copyright:	BSD
Group:		System Environment/Daemons
Source:		http://prdownloads.sourceforge.net/qico/qico-%{version}.tar.bz2
URL:		http://qico.sourceforge.net/
BuildRoot:	%{_tmppath}/%{name}-%{version}-root
Prefix:		%{_prefix}

%description
Qico is an FidoNet Technology Network (FTN) compatible mailer for Unix
systems. It has some original features. Full list of features you can
find in README, Changes and qico.conf files.

%prep
%setup -q

%build
%configure
make

%install
rm -rf %{buildroot}
%makeinstall
mkdir %{buildroot}/etc
cp qico.conf.sample %{buildroot}/etc/qico.conf
mkdir -p %{buildroot}/etc/rc.d/init.d
install -m 755 stuff/ftn %{buildroot}/etc/rc.d/init.d/qico

%clean
rm -rf %{buildroot}

%post
/sbin/chkconfig --add qico

%preun
if [ $1 = 0 ]; then
   /sbin/chkconfig --del qico
fi

%files
%defattr(-,root,root)
%{_sbindir}/qico
%{_bindir}/qctl
%{_bindir}/qcc
%config /etc/qico.conf
%config /etc/rc.d/init.d/qico
%doc stuff README FAQ Changes
%doc %{_mandir}/man8/qico.8*
%doc %{_mandir}/man8/qctl.8*
%doc %{_mandir}/man8/qcc.8*

%changelog
* Sat May 29 2004 sisoft <sisoft@bk.ru>
  [0.53si-1]
- ported to current version.
* Wed Sep 27 2000 pqr <pqr@yasp.com>
  [0.49.0-1]
- some fixes about prefix and others
* Tue Aug 29 2000 pqr <pqr@yasp.com>
  [0.47-1]
- initial version

