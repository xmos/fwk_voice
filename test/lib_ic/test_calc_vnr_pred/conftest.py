
def pytest_generate_tests(metafunc):
    if "target" in metafunc.fixturenames:
        metafunc.parametrize("target", ['native', 'xs3a'])
