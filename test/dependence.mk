$(BIN)/printabbr : $(BIN)/test.o $(BIN)/printabbr.o

$(BIN)/test_context : $(BIN)/test_context.o $(BIN)/context.o $(BIN)/dicts.o $(BIN)/reader.o $(BIN)/test.o

OBJS += $(BIN)/printabbr $(BIN)/test_context

