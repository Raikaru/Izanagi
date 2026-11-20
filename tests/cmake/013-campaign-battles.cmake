# The Challenge campaign battle inventory (tools/campaign_battles.py) on synthetic
# flowscripts, a synthetic ELF command table and encounter records; no retail data.
add_test(NAME campaign_battles COMMAND "${P4_PYTHON}" "${CMAKE_CURRENT_SOURCE_DIR}/campaign_battles_test.py")
