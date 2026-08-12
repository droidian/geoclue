if get_option('enable-backend')
    conf = configuration_data()
    conf.set('sysconfdir', sysconfdir)

    if get_option('demo-agent')
        conf.set('demo_agent', 'geoclue-demo-agent;')
    else
        conf.set('demo_agent', '')
    endif

    if get_option('default-wifi-submit-url') != ''
        conf.set('default_wifi_submit_url', get_option('default-wifi-submit-url'))
    else
        conf.set('default_wifi_submit_url', '<UNSET>')
    endif

    if get_option('default-wifi-url') != ''
        conf.set('default_wifi_enable', 'true')
        conf.set('default_wifi_url', get_option('default-wifi-url'))
    else
        conf.set('default_wifi_enable', 'false')
        conf.set('default_wifi_url', '<UNSET>')
    endif

    conf_dir = join_paths(sysconfdir, 'geoclue')
    configure_file(output: 'geoclue.conf',
                   input: 'geoclue.conf.in',
                   configuration: conf,
                   install_dir: conf_dir)

    dbus_srv_user = get_option('dbus-srv-user')

    conf = configuration_data()
    conf.set('libexecdir', libexecdir)
    conf.set('dbus_srv_user', dbus_srv_user)
    conf.set('sysconfdir', sysconfdir)

    confd_dir = join_paths(conf_dir, 'conf.d')
    install_emptydir(confd_dir)

    service_dir = join_paths(datadir, 'dbus-1', 'system-services')
    configure_file(output: 'org.freedesktop.GeoClue2.service',
                   input: 'org.freedesktop.GeoClue2.service.in',
                   configuration: conf,
                   install_dir: service_dir)

    man_file = configure_file(output: 'geoclue.5',
                              input: 'geoclue.5.in',
                              configuration: conf)

    install_man(man_file)

    # DBus Service policy file
    dbus_service_dir = get_option('dbus-sys-dir')
    if dbus_service_dir == ''
        dbus_service_dir = join_paths(datadir, 'dbus-1', 'system.d')
    endif
    configure_file(output: 'org.freedesktop.GeoClue2.conf',
                   input: 'org.freedesktop.GeoClue2.conf.in',
                   configuration: conf,
                   install_dir: dbus_service_dir)
    configure_file(output: 'org.freedesktop.GeoClue2.Agent.conf',
                   input: 'org.freedesktop.GeoClue2.Agent.conf.in',
                   configuration: conf,
                   install_dir: dbus_service_dir)

    systemd = dependency('systemd', required: false)
    if dbus_srv_user != 'root'
        if systemd.found()
            sysusers_dir = systemd.get_variable(pkgconfig: 'sysusersdir')
        else
            sysusers_dir = '/usr/lib/sysusers.d'
        endif
        configure_file(output: 'geoclue-sysusers.conf',
                       input: 'geoclue-sysusers.conf.in',
                       configuration: conf,
                       install_dir: sysusers_dir)
    endif

    systemd_unit_dir = get_option('systemd-system-unit-dir')
    if systemd_unit_dir == ''
        if systemd.found()
            systemd_unit_dir = systemd.get_variable(pkgconfig: 'systemdsystemunitdir')
        endif
    endif

    if systemd_unit_dir != ''
        configure_file(output: 'geoclue.service',
                       input: 'geoclue.service.in',
                       configuration: conf,
                       install_dir: systemd_unit_dir)
    endif

    if get_option('modem-gps-source')
        polkit_dir = join_paths(get_option('datadir'), 'polkit-1', 'rules.d')
        configure_file(output: 'org.freedesktop.GeoClue2.rules',
                       input: 'org.freedesktop.GeoClue2.rules.in',
                       configuration: conf,
                       install_dir: polkit_dir)
    endif
endif
