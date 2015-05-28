%define PKG_PREFIX org.tizen

Name:       org.tizen.music-player
Summary:    music player application
Version:    0.1.177
Release:    1
License:    Flora
Source0:    %{name}-%{version}.tar.gz
BuildRequires:  pkgconfig(capi-appfw-application)
BuildRequires:  pkgconfig(elementary)
BuildRequires:  pkgconfig(ecore)
BuildRequires:  pkgconfig(capi-media-sound-manager)
BuildRequires:  pkgconfig(capi-media-player)
BuildRequires:  pkgconfig(capi-media-metadata-extractor)
BuildRequires:  pkgconfig(dlog)
BuildRequires:  pkgconfig(drm-client)
BuildRequires:  pkgconfig(ui-gadget-1)
BuildRequires:  pkgconfig(syspopup-caller)
BuildRequires:  pkgconfig(capi-system-power)
BuildRequires:  pkgconfig(libxml-2.0)
BuildRequires:  pkgconfig(eina)
BuildRequires:  pkgconfig(ecore-imf)
BuildRequires:  pkgconfig(ecore-x)
BuildRequires:  pkgconfig(ecore-file)
BuildRequires:  pkgconfig(ecore-input)
BuildRequires:  pkgconfig(libcrypto)
BuildRequires:  pkgconfig(edje)
BuildRequires:  pkgconfig(capi-appfw-app-manager)
BuildRequires:  pkgconfig(evas)
BuildRequires:  pkgconfig(glib-2.0)
BuildRequires:  pkgconfig(minicontrol-provider)
BuildRequires:  pkgconfig(capi-system-media-key)
BuildRequires:  pkgconfig(xext)
BuildRequires:  pkgconfig(capi-appfw-application)
BuildRequires:  pkgconfig(capi-content-media-content)
BuildRequires:  pkgconfig(capi-network-bluetooth)
BuildRequires:  pkgconfig(capi-system-system-settings)
BuildRequires:  pkgconfig(utilX)
BuildRequires:  pkgconfig(sqlite3)
BuildRequires:  pkgconfig(notification)
BuildRequires:  cmake
BuildRequires:  prelink
BuildRequires:  edje-tools
BuildRequires:  gettext-tools
Requires:  media-server email-service media-data-sdk ug-bluetooth-efl

%description
music player application.

%package -n %{PKG_PREFIX}.sound-player
Summary:    Sound player

%description -n %{PKG_PREFIX}.sound-player
Description: sound player application

#Requires:   %{name} = %{version}-%{release}

%package -n ug-music-player-efl
Summary:    music-player UG
Group:      TO_BE/FILLED_IN

%description -n ug-music-player-efl
Description: music-player UG


%prep
%setup -q

%define DESKTOP_DIR /usr/share
%define INSTALL_DIR	/usr/apps
%define DATA_DIR	/opt/usr/apps

%define PKG_NAME %{name}
%define PREFIX %{INSTALL_DIR}/%{PKG_NAME}
%define DATA_PREFIX %{DATA_DIR}/%{PKG_NAME}

%define SP_PKG_NAME %{PKG_PREFIX}.sound-player
%define SP_PREFIX %{INSTALL_DIR}/%{SP_PKG_NAME}
%define SP_DATA_PREFIX %{DATA_DIR}/%{SP_PKG_NAME}

%define UG_PREFIX /usr/ug

%build
cmake . -DUG_PREFIX="%{UG_PREFIX}" -DINSTALL_DIR="%{INSTALL_DIR}" -DCMAKE_INSTALL_PREFIX="%{PREFIX}" -DCMAKE_DESKTOP_ICON_DIR="%{DESKTOP_DIR}/icons/default/small" -DDESKTOP_DIR="%{DESKTOP_DIR}" -DPKG_NAME="%{PKG_NAME}" -DSP_PKG_NAME="%{SP_PKG_NAME}" -DDATA_PREFIX="%{DATA_PREFIX}" -DSP_DATA_PREFIX="%{SP_DATA_PREFIX}"
make %{?jobs:-j%jobs}

%install
rm -rf %{buildroot}
mkdir -p %{buildroot}/usr/share/license
cp LICENSE.Flora %{buildroot}/usr/share/license/%{name}

%make_install

execstack -c %{buildroot}%{PREFIX}/bin/music-player

%pre
if [ -n "`env|grep SBOX`" ]; then
        echo "postinst: sbox installation"
else
        PID=`/bin/pidof music-player`
        if [ -n "$PID" ]; then
                echo "preinst: kill current music-player app"
                /usr/bin/killall music-player
        fi
        PID=`/bin/pidof sound-player`
        if [ -n "$PID" ]; then
                echo "preinst: kill current sound-player app"
                /usr/bin/killall sound-player
        fi
fi

%post
mkdir -p %{DATA_PREFIX}/data
chown -R 5000:5000 %{DATA_PREFIX}/data
mkdir -p %{SP_DATA_PREFIX}/data
chown -R 5000:5000 %{SP_DATA_PREFIX}/data

mkdir -p /usr/ug/bin/
ln -sf /usr/bin/ug-client /usr/ug/bin/music-player-efl

/usr/bin/vconftool set -t int memory/music/state 0 -i -g 5000

/usr/bin/vconftool set -t string memory/private/org.tizen.music-player/pos "00:00" -i -g 5000
/usr/bin/vconftool set -t double memory/private/org.tizen.music-player/progress_pos 0.0 -i -g 5000
/usr/bin/vconftool set -t double memory/private/org.tizen.music-player/position_changed 0.0 -i -g 5000
/usr/bin/vconftool set -t bool memory/private/org.tizen.music-player/player_state 1 -i -g 5000
/usr/bin/vconftool set -t bool memory/private/org.tizen.music-player/play_clicked 1 -i -g 5000
/usr/bin/vconftool set -t bool memory/private/org.tizen.music-player/pause_clicked 1 -i -g 5000
/usr/bin/vconftool set -t bool memory/private/org.tizen.music-player/prev_pressed 1 -i -g 5000
/usr/bin/vconftool set -t bool memory/private/org.tizen.music-player/prev_released 1 -i -g 5000
/usr/bin/vconftool set -t bool memory/private/org.tizen.music-player/next_pressed 1 -i -g 5000
/usr/bin/vconftool set -t bool memory/private/org.tizen.music-player/next_released 1 -i -g 5000
/usr/bin/vconftool set -t double memory/private/org.tizen.music-player/playspeed 1.0 -i -g 5000
/usr/bin/vconftool set -t int memory/private/org.tizen.music-player/auto_off_time_val 0 -i -g 5000
/usr/bin/vconftool set -t int memory/private/org.tizen.music-player/auto_off_type_val 0 -i -g 5000

/usr/bin/vconftool set -t bool db/private/org.tizen.music-player/shuffle 1 -g 5000
/usr/bin/vconftool set -t int db/private/org.tizen.music-player/repeat 0 -g 5000
/usr/bin/vconftool set -t int db/private/org.tizen.music-player/square_axis_val 0 -g 5000
/usr/bin/vconftool set -t int db/private/org.tizen.music-player/playlist 7 -g 5000

/usr/bin/vconftool set -t int memory/private/org.tizen.music-player/playing_pid 0 -i -g 5000


%files
%manifest %{name}.manifest
%{DESKTOP_DIR}/packages/%{name}.xml
%{DESKTOP_DIR}/icons/default/small/%{name}.png
%{PREFIX}/bin/music-player
%{PREFIX}/res/locale/*/LC_MESSAGES/*.mo
%{PREFIX}/res/images/*
%{PREFIX}/res/edje/*.edj
/opt/etc/smack/accesses.d/org.tizen.music-player.rule
/usr/share/license/%{name}

%{DESKTOP_DIR}/icons/default/small/%{SP_PKG_NAME}.png
%{SP_PREFIX}/bin/sound-player

%files -n ug-music-player-efl
%manifest ug-music-player-efl.manifest
%{DESKTOP_DIR}/packages/ug-music-player-efl.xml
%defattr(-,root,root,-)
%{UG_PREFIX}/lib/libug-music-player-efl.so
%{UG_PREFIX}/lib/libug-music-player-efl.so.0.1.0
%{UG_PREFIX}/res/edje/ug-music-player-efl/ug-music-player-efl.edj
