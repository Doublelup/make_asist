.SUFFIXS:
.PHONY : default force test
.DEFAULT_GOAL := default

W_DIR = /home/lup/rubbish/make_helper
TEST_DIR = $(W_DIR)/test
ABBR = $(W_DIR)/abbr.py
ABBR_FILE ?= $(TEST_DIR)/rule.abbr
EXTEND_FILE ?= $(TEST_DIR)/dependence.mk
RULE_MK ?= $(W_DIR)/default.mk
INCLUDE_MK = $(EXTEND_FILE)
TEST_SCRIPT ?= $(TEST_DIR)/test.sh

default : $(EXTEND_FILE) force;

$(EXTEND_FILE) : $(ABBR_FILE)
	$(ABBR) $(ABBR_FILE) $(EXTEND_FILE)

force : ;
	$(MAKE) -f $(RULE_MK) INCLUDE_MK='$(INCLUDE_MK)'

clean :;
	$(MAKE) -f $(RULE_MK) clean

test :;
	$(MAKE) -f $(RULE_MK) test TEST_SCRIPT='$(TEST_SCRIPT)'
	







