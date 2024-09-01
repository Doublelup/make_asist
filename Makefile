.SUFFIXES:
.PHONY : default force
.DEFAULT_GOAL := default

W_DIR != pwd
ABBR ?= $(W_DIR)/abbr.py
ABBR_FILE ?= $(W_DIR)/rule.abbr
EXTEND_FILE ?= $(W_DIR)/bin/dependence.mk
RULE_MK ?= $(W_DIR)/default.mk
INCLUDE_MK = $(EXTEND_FILE)

default : $(EXTEND_FILE) force;

$(EXTEND_FILE) : $(ABBR_FILE)
	$(ABBR) $(ABBR_FILE) $(EXTEND_FILE)

force: ;
	$(MAKE) -f $(RULE_MK) INCLUDE_MK='$(INCLUDE_MK)'

clean :;
	$(MAKE) -f $(RULE_MK) clean

