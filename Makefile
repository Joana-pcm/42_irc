NAME		= ircserv
CC			= c++
RM			= rm -f
CPPFLAGS	= -Wall -Werror -Wextra -std=c++98 -g
SRC			= $(addprefix srcs/, Server.cpp main.cpp Client.cpp Channel.cpp Message.cpp Commands.cpp)

OBJ			= ${SRC:.cpp=.o}

.cpp.o:
	${CC} ${CPPFLAGS} -c $< -o ${<:.cpp=.o}

${NAME}: ${OBJ}
	@${CC} ${OBJ} ${CPPFLAGS} -o ${NAME}

all: ${NAME}

clean:
	@${RM} ${OBJ}

fclean: clean
	@${RM} ${NAME}

re: fclean all

.PHONY: all clean fclean re