NAME     = ircserv
CXX		 = c++
CXXFLAGS = -Wall -Wextra -Werror -std=c++98 -MMD -MP -pedantic
SRC		 = src/main.cpp src/Server.cpp src/Client.cpp \
		   src/Channel.cpp src/CommandUtils.cpp src/Parser.cpp \
		   src/Command.cpp src/CommandHandler.cpp \
		   src/ChannelCommands.cpp
OBJ_DIR  = obj
OBJ      = $(SRC:src/%.cpp=$(OBJ_DIR)/%.o)
DEP      = $(OBJ:.o=.d)
RM 		 = rm -f

all: $(NAME)

$(NAME): $(OBJ)
	$(CXX) $(OBJ) $(CXXFLAGS) -o $@

$(OBJ_DIR)/%.o: src/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

-include $(DEP)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

debug:
	$(MAKE) CXXFLAGS="$(CXXFLAGS) -g" re

valgrind: re
	valgrind --leak-check=full --show-leak-kinds=all \
	         --track-origins=yes  ./$(NAME)

run: all
	./$(NAME)
	
clean:
	rm -rf $(OBJ_DIR)

fclean: clean
	$(RM) $(NAME)

re: fclean all

.PHONY	: all clean fclean re