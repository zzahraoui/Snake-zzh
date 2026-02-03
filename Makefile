# Compilateur et flags
CC = C:/msys64/ucrt64/bin/gcc.exe
CFLAGS = -Wall -Wextra -std=c11 -I include -IC:/msys64/ucrt64/include
# Ajout de SDL2_mixer pour le son
LDFLAGS = -LC:/msys64/ucrt64/lib -lmingw32 -lSDL2main -lSDL2 -lSDL2_ttf -lSDL2_mixer -lm

# Dossiers
SRC_DIR = src
BUILD_DIR = build
TARGET = $(BUILD_DIR)/snake.exe

# Fichiers sources
SRCS = $(wildcard $(SRC_DIR)/*.c)
OBJS = $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(SRCS))

# Règle par défaut
all: $(BUILD_DIR) $(TARGET)

# Créer le dossier build s'il n'existe pas
$(BUILD_DIR):
	@if not exist "$(BUILD_DIR)" mkdir $(BUILD_DIR)

# Lier tous les objets pour créer l'exécutable
$(TARGET): $(OBJS)
	@echo Linking...
	$(CC) $(OBJS) -o $(TARGET) $(LDFLAGS)
	@echo Build terminé: $(TARGET)

# Compiler chaque fichier .c en .o
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@echo Compiling $<...
	$(CC) $(CFLAGS) -c $< -o $@

# Nettoyer les fichiers compilés
clean:
	@if exist "$(BUILD_DIR)" rmdir /s /q $(BUILD_DIR)
	@echo Nettoyage terminé

# Compiler et exécuter
run: all
	@echo Exécution du programme...
	@$(TARGET)

# Forcer la recompilation
rebuild: clean all

.PHONY: all clean run rebuild