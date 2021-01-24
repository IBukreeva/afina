#include <afina/coroutine/Engine.h>

#include <setjmp.h>
#include <stdio.h>
#include <string.h>

namespace Afina {
namespace Coroutine {

Engine::~Engine() { 
    //we need to delete all alive and blocked coros

    auto &routine = alive;
    while (routine != nullptr) {
        auto &tmp = routine;
        delete[] std::get<0>(tmp->Stack);
        routine = routine->next;
        delete tmp;  
    }

    routine = blocked;
    while (routine != nullptr) {
        auto &tmp = routine;
        delete[] std::get<0>(tmp->Stack);
        routine = routine->next;
        delete tmp;  
    }
}

void Engine::Store(context &ctx) {
    char tmp;

    if (&tmp > ctx.Low) { // Hight > Low
        ctx.Hight = &tmp;
    } else { // Hight < Low
        ctx.Low = &tmp;
    }

    auto stack_size = ctx.Hight - ctx.Low;
    if(std::get<1>(ctx.Stack) < stack_size || std::get<1>(ctx.Stack) > 2*stack_size) {
        delete[] std::get<0>(ctx.Stack);
        std::get<0>(ctx.Stack) = new char[stack_size];
        std::get<1>(ctx.Stack) = stack_size;
    }

    memcpy(std::get<0>(ctx.Stack), ctx.Low, stack_size);

}

void Engine::Restore(context &ctx) {
    char tmp;
    while(&tmp <= ctx.Hight && &tmp >= ctx.Low) { //between Low and Hight
        Restore(ctx);
    }

    std::size_t stack_size = ctx.Hight - ctx.Low;
    memcpy(ctx.Low, std::get<0>(ctx.Stack), stack_size);
    cur_routine = &ctx;
    longjmp(ctx.Environment, 1);
}

void Engine::yield() {
    sched(nullptr);
}

void Engine::sched(void *routine_) {

    context *coro_to_switch;
    if (routine_ == nullptr) { // switch to any alive coroutine
        //but before it we need to check if we have any alive coro to switch  
        if (alive==nullptr) {
            return;
        } else if (alive->next == nullptr) {
            if (cur_routine == alive) {
                return;
            }
        }  
        // so neither there is no next and we can switch to alive
        // neither alive->next and alive exist and we can switch to one of them
        if (cur_routine == alive) { // then alive->next exists we checked it just now
            coro_to_switch = alive->next;
        } else {
            coro_to_switch = alive;
        }
    } else { //switch to routine_ but not so simple
        coro_to_switch = static_cast<context *>(routine_);

        if (cur_routine == coro_to_switch || coro_to_switch->is_blocked) {
            return;
        }
    }

    // finally switch to coro_to_switch
    if (cur_routine != idle_ctx) {
        if (setjmp(cur_routine->Environment) > 0) {
            return;
        }
        Store(*cur_routine);
    }
    Restore(*coro_to_switch);
}

void Engine::block(void *coro) {

    //choose coro to block:
    context *coro_to_block = nullptr;

    if (coro == nullptr) {
        coro_to_block = cur_routine;
    } else {
        coro_to_block = static_cast<context *>(coro);
    }
    //block coro_to_block
    if (coro_to_block->is_blocked) {
        return;
    }
    coro_to_block->is_blocked = true;

    //delete from list of alive coros
    if (alive == coro_to_block) {
        alive = alive->next;
    }
    if (coro_to_block->next) {
        coro_to_block->prev->next = coro_to_block->next;
    }
    if (coro_to_block->prev) {
        coro_to_block->next->prev = coro_to_block->prev;
    }
    //add to list of blocked coros
    coro_to_block->next = blocked;
    coro_to_block->prev = nullptr;
    blocked = coro_to_block;
    if (blocked->next) {
        blocked->next->prev = blocked;
    }
    if (coro_to_block == cur_routine) { // maybe it was cur_routine? it's possible
        if (setjmp(cur_routine->Environment) > 0) {
            return;
        }
        Store(*cur_routine);
        Restore(*idle_ctx); // then go to idle_ctx
    }
}

void Engine::unblock(void *coro){
    // choose coro to unblock - there is no choise because we can't unblock coro_ in coro_
    context *coro_to_unblock = static_cast<context *>(coro);
    
    //unblock coro_to_unblock
    if (coro_to_unblock->is_blocked == false) {
        return;
    }
    coro_to_unblock->is_blocked = false;
    
    //add to list of alive coros
    coro_to_unblock->next = alive;
    coro_to_unblock->prev = nullptr;
    alive = coro_to_unblock;
    if (alive->next) {
        alive->next->prev = alive;
    }

    //delete from list of blocked coros
    if (blocked == coro_to_unblock) {
        blocked = blocked->next;
    }
    if (coro_to_unblock->next) {
        coro_to_unblock->prev->next = coro_to_unblock->next;
    }
    if (coro_to_unblock->prev) {
        coro_to_unblock->next->prev = coro_to_unblock->prev;
    }
}

} // namespace Coroutine
} // namespace Afina
