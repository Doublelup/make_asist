$(BIN)/test_dicts : $(BIN)/test_dicts.o $(BIN)/dicts.o

$(BIN)/test_reader : $(BIN)/test_reader.o $(BIN)/temp_context.o $(BIN)/reader.o $(BIN)/dicts.o $(BIN)/test.o

$(BIN)/printabbr : $(BIN)/test.o $(BIN)/printabbr.o

OBJS += $(BIN)/test_dicts

OBJS += $(BIN)/test_reader $(BIN)/printabbr

