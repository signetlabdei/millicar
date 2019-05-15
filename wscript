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
        'helper/mmwave-vehicular-helper.cc',
        ]

    module_test = bld.create_ns3_module_test_library('mmwave-vehicular')
    module_test.source = [
        'test/mmwave-vehicular-test-suite.cc',
        ]

    headers = bld(features='ns3header')
    headers.module = 'mmwave-vehicular'
    headers.source = [
        'model/mmwave-vehicular.h',
        'model/mmwave-vehicular-propagation-loss-model.h',
        'model/mmwave-vehicular-spectrum-propagation-loss-model.h',
        'helper/mmwave-vehicular-helper.h',
        ]

    if bld.env.ENABLE_EXAMPLES:
        bld.recurse('examples')

    # bld.ns3_python_bindings()
