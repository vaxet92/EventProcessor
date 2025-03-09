
#Compiler and Linker
CXX = g++

#The Target Binary Program
TARGET = main

#The Directories, Source, Includes, Objects, Binary and Resources
WS_DIR = ws
SRC_DIR = src


MES_OBJDIR = $(MES_DIR)/objs
MES_SRCS = $(wildcard $(MES_DIR)/*.cpp)
MES_OBJS = $(addprefix $(MES_OBJDIR)/, $(notdir $(MES_SRCS:.cpp=.o)))

SRC_OBJDIR = $(SRC_DIR)/objs
SRC_SRCS = $(wildcard $(SRC_DIR)/*.cpp)
SRC_OBJS = $(addprefix $(SRC_OBJDIR)/, $(notdir $(SRC_SRCS:.cpp=.o)))

SIMDJSON_OBJDIR = $(SIMDJSON_DIR)/objs
SIMDJSON_SRCS = $(wildcard $(SIMDJSON_DIR)/*.cpp)
SIMDJSON_OBJS = $(addprefix $(SIMDJSON_OBJDIR)/, $(notdir $(SIMDJSON_SRCS:.cpp=.o)))

OBJDIR = objs
TARGETDIR = build


# BOOST_PATH = ../libraries/boost_1_82_0/boost
# BOOST_INCLUDE_PATH =  $(LIBRARIES_DIR)/boost_libs/include
# BOOST_LIB_PATH =  $(LIBRARIES_DIR)/boost_libs/lib
# SIMDJSON_DIR =  $(LIBRARIES_DIR)/simdjson

INCDIR = -I. 
# INCDIR += -I$(LIBRARIES_DIR)
# INCDIR += -I$(FMT_PATH)
INCDIR += -I$(SRC_DIR)



#Flags, Libraries and Includes
CXXFLAGS = -std=c++20
CXXFLAGS += -O0
# CXXFLAGS += -O3
CXXFLAGS += -Wall
CXXFLAGS += -Wextra
CXXFLAGS += -Wpedantic
CXXFLAGS += -fdiagnostics-color=always
CXXFLAGS += -fconcepts
CXXFLAGS += -g
CXXFLAGS += -ggdb3
CXXFLAGS += $(INCDIR)

CXXFLAGS += -fsanitize=address

# Linker flags
# LDFLAGS	= -L$(BOOST_LIB_PATH)
# LDFLAGS += -L$(LIBDIR)
# LDFLAGS += -lsimdjson
LDFLAGS += -lpthread
LDFLAGS += -fsanitize=address




# Source files
MAIN_SRCS = main.cpp

# Object files
MAIN_OBJS = $(addprefix $(OBJDIR)/, $(notdir $(MAIN_SRCS:.cpp=.o)))

# Build rule for the target
$(TARGET): $(SRC_OBJS) $(MAIN_OBJS) 
	$(CXX) $(CXXFLAGS) -o $(TARGETDIR)/$@ $^ $(LDFLAGS)


# Build rule for individual source files
$(OBJDIR)/%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $^


# Generate a list of object files from source files
WS_OBJS = $(addprefix $(WS_OBJDIR)/, $(notdir $(WS_SRCS:.cpp=.o)))

# Rule to build the static library
libws.a: $(WS_OBJS) $(MES_OBJS)
	ar rcs $(LIBDIR)/$@ $^

# Rule to build object files
$(WS_OBJDIR)/%.o: $(WS_DIR)/%.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $^
#---------------------------------------------------------------------------------

$(MES_OBJS): $(MES_SRCS)
	$(CXX) $(CXXFLAGS) -c -o $@ $<


#------------------sources----------------------------------------------

$(SRC_OBJDIR)/event_proc.o: $(SRC_DIR)/event_proc.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $^

#------------------simdjson lib----------------------------------------------

# Rule to build the static library
libsimdjson.a: $(SIMDJSON_OBJS)
	ar rcs $(LIBDIR)/$@ $^

# Rule to build object files
$(SIMDJSON_OBJS): $(SIMDJSON_SRCS)
	$(CXX) $(CXXFLAGS) -c -o $@ $^
#---------------------------------------------------------------------------------

libraries: libsimdjson.a libws.a


# #Remake
remake: cleaner all

#Make the Directories
directories:
	@mkdir -p $(TARGETDIR)
	@mkdir -p $(OBJDIR)
	@mkdir -p $(LIBDIR)
	@mkdir -p $(WS_DIR)/$(OBJDIR)
	@mkdir -p $(SRC_DIR)/$(OBJDIR)
	@mkdir -p $(SIMDJSON_DIR)/$(OBJDIR)

#Clean only Objecst
clean:
	@$(RM) -f $(TARGETDIR)/*.$(TARGET)
	@$(RM) -f $(MAIN_OBJS)
	@$(RM) -f $(SRC_OBJS)
	@$(RM) -f $(WS_OBJS)
