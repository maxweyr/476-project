#include "Player.h"

Player::Player(const glm::vec3& position, float hitpoints, float moveSpeed)
    : Entity(position, hitpoints, moveSpeed) {}

SpellSlot Player::getActiveSpellSlot() {
    return activeSpellSlot;
}

void Player::pickupSpell(Spell *spell, SpellSlot activeSpellSlot) {
    
    // Put spell in inventory
    if (isInventoryFull()) {
        for (int i = 0; i < INVENTORY_SIZE; i++) {
            if (spellInventory[i].getType() == SpellType::NO_SPELL) {
                spellInventory[i] = *spell;
                return;
            }
        }
    } else {
        // If inventory is full, replace the spell in the active slot
        /* Logic to drop the spell in the active slot ?? */
        spellInventory[activeSpellSlot] = *spell;
    }
}

bool Player::isInventoryFull() {
    for (int i = 0; i < INVENTORY_SIZE; i++) {
        if (spellInventory[i].getType() == SpellType::NO_SPELL) {
            return false;
        }
    }
    return true;
}

Spell Player::getSpellFromSpellSlot(SpellSlot slot) {
    return spellInventory[slot];
}

void Player::setSprintFlag(bool flag) {
    sprintFlag = flag;
}

void Player::move(const glm::vec3& direction) {
    if (sprintFlag) {
        Entity::move(direction * 2.0f);     // arbitrary sprint speed
    } else {
        Entity::move(direction);
    }
}

void Player::castSpell() {
    if (currentSpell->getType() != SpellType::NO_SPELL) {
        currentSpell->cast();
    }
}

void initPlayer() {}