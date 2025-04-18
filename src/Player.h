#ifndef PLAYER_H
#define PLAYER_H

#include "Entity.h"
#include "Spell.h"

#define INVENTORY_SIZE 8

typedef enum {
    SLOT_LEFT,
    SLOT_RIGHT,
    SLOT_INV_ONE,
    SLOT_INV_TWO,
    SLOT_INV_THREE,
    SLOT_INV_FOUR,
    SLOT_INV_FIVE,
    SLOT_INV_SIX
} SpellSlot;

class Player : public Entity {
    private:
        Spell *currentSpell;

        // Init spell inventory with empty slots
        Spell spellInventory[INVENTORY_SIZE] = {
            Spell("Empty", SpellType::NO_SPELL, 0, 0, 0, 0, SpellType::NO_SPELL),
            Spell("Empty", SpellType::NO_SPELL, 0, 0, 0, 0, SpellType::NO_SPELL),
            Spell("Empty", SpellType::NO_SPELL, 0, 0, 0, 0, SpellType::NO_SPELL),
            Spell("Empty", SpellType::NO_SPELL, 0, 0, 0, 0, SpellType::NO_SPELL),
            Spell("Empty", SpellType::NO_SPELL, 0, 0, 0, 0, SpellType::NO_SPELL),
            Spell("Empty", SpellType::NO_SPELL, 0, 0, 0, 0, SpellType::NO_SPELL),
            Spell("Empty", SpellType::NO_SPELL, 0, 0, 0, 0, SpellType::NO_SPELL),
            Spell("Empty", SpellType::NO_SPELL, 0, 0, 0, 0, SpellType::NO_SPELL)
        };

        SpellSlot activeSpellSlot;
        bool sprintFlag;
        bool dodgeFlag;

    public:
        Player(const glm::vec3& position, float hitpoints, float moveSpeed);

        void move(const glm::vec3& direction);
        SpellSlot getActiveSpellSlot();
        void setCurrentSpellSlot(SpellSlot slot);
        Spell getSpellFromSpellSlot(SpellSlot slot);
        void pickupSpell(Spell *spell, SpellSlot activeSpellSlot);
        void castSpell();
        void setSprintFlag(bool flag);
        void dodge();
        bool isInventoryFull();
};

void initPlayer();

#endif // PLAYER_H