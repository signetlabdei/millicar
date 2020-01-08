# -*- Mode: python; py-indent-offset: 4; indent-tabs-mode: nil; coding: utf-8; -*-

# def options(opt):
#     pass

# def configure(conf):
#     conf.check_nonfatal(header_name='stdint.h', define_name='HAVE_STDINT_H')

def build(bld):
    module = bld.create_ns3_module('mmwave-vehicular', ['core', 'propagation', 'spectrum', 'mmwave'])
    module.source = [
        'model/mmwave-vehicular.cc',
        'model/mmwave-vehicular-propagation-loss-model.cc',
        'model/mmwave-vehicular-spectrum-propagation-loss-model.cc',
        'model/mmwave-sidelink-spectrum-phy.cc',
        'model/mmwave-sidelink-spectrum-signal-parameters.cc',
        'model/mmwave-sidelink-phy.cc',
        'model/mmwave-sidelink-mac.cc',
        'model/mmwave-vehicular-net-device.cc',
        'helper/mmwave-vehicular-helper.cc',
        ]

    module_test = bld.create_ns3_module_test_library('mmwave-vehicular')
    module_test.source = [
        'test/mmwave-vehicular-test-suite.cc',
        'test/mmwave-vehicular-sidelink-spectrum-phy-test.cc',
        'test/mmwave-sidelink-phy-test-suite.cc',
        'test/mmwave-vehicular-rate-test.cc'
        ]

    headers = bld(features='ns3header')
    headers.module = 'mmwave-vehicular'
    headers.source = [
        'model/mmwave-vehicular.h',
        'model/mmwave-vehicular-propagation-loss-model.h',
        'model/mmwave-vehicular-spectrum-propagation-loss-model.h',
        'model/mmwave-sidelink-spectrum-phy.h',
        'model/mmwave-sidelink-spectrum-signal-parameters.h',
        'model/mmwave-sidelink-phy.h',
        'model/mmwave-sidelink-mac.h',
        'model/mmwave-sidelink-sap.h',
        'model/mmwave-vehicular-net-device.h',
        'helper/mmwave-vehicular-helper.h',
        ]

    if bld.env.ENABLE_EXAMPLES:
        bld.recurse('examples')

    # bld.ns3_python_bindings()
