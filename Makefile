CXX = g++
RM = rm

CFLAGS = `pkg-config --cflags opencv`
LIBS = `pkg-config --libs opencv`

TARGET = traffic

OBJS = camera.o main.o

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) -o $@ $^ $(CFLAGS) $(LIBS)
	
%.o: %.cpp
	$(CXX) -c $(CFLAGS) $<

clean: 
	$(RM) -f *.o $(TARGET)
