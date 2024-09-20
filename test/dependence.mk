$(BIN)/test_reader : $(BIN)/test_reader.o $(BIN)/reader.o $(BIN)/dicts.o $(BIN)/test.o $(BIN)/context.o

$(BIN)/printabbr : $(BIN)/test.o $(BIN)/printabbr.o

$(BIN)/make_asist : $(BIN)/reader.o $(BIN)/dicts.o $(BIN)/context.o $(BIN)/main.o

OBJS += $(BIN)/printabbr $(BIN)/test_reader $(BIN)/make_asist

