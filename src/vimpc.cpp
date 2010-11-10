/*
   Vimpc
   Copyright (C) 2010 Nathan Sweetman

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.

   vimpc.hpp - handles mode changes and input processing
   */

#include "vimpc.hpp"

// Mode handlers
#include "handler.hpp"
#include "actions.hpp"
#include "commands.hpp"
#include "search.hpp"

#include "config.hpp"
#include "dbc.hpp"
#include "settings.hpp"

using namespace Main;

Vimpc::Vimpc() :
   currentMode_ (Normal),
   handlerTable_(),
   settings_    (Main::Settings::Instance()),
   screen_      (client_, settings_), // \todo surely this use of screen_/client_ coupling is bad
   client_      (screen_)
{
   handlerTable_[Normal]  = new Ui::Actions (screen_, client_);
   handlerTable_[Command] = new Ui::Commands(screen_, client_, settings_);
   handlerTable_[Search]  = new Ui::Search  (screen_, client_, settings_);

   ENSURE(handlerTable_.size() == ModeCount);
}

Vimpc::~Vimpc()
{
   for (HandlerTable::iterator it = handlerTable_.begin(); (it != handlerTable_.end()); ++it)
   {
      delete (it->second);
      it->second = NULL;
   }
}

void Vimpc::Run()
{
   Ui::Commands * commandHandler = dynamic_cast<Ui::Commands *>(handlerTable_[Command]);

   ASSERT(commandHandler != NULL);

   if (Config::ExecuteConfigCommands(*commandHandler) == true)
   {
      client_.Start();
      screen_.Start();

      handlerTable_[currentMode_]->InitialiseMode();

      while (Handle(Input()) == true);
   }
}


int Vimpc::Input() const
{
   return screen_.WaitForInput();
}

bool Vimpc::Handle(int input)
{
   Ui::Handler * handler = handlerTable_[currentMode_];

   ASSERT(handler != NULL);

   // Input must be handled before mode is changed
   bool const result = handler->Handle(input);

   if (RequiresModeChange(input) == true)
   {
      ChangeMode(input);
   }

   return result;
}

bool Vimpc::RequiresModeChange(int input) const
{
   return (currentMode_ != ModeAfterInput(input));
}

Vimpc::Mode Vimpc::ModeAfterInput(int input) const
{
   Mode newMode = currentMode_;

   // Check if we are returning to normal mode
   if (currentMode_ != Normal)
   {
      if (handlerTable_.at(Normal)->CausesModeStart(input) == true)
      {   
         newMode = Normal;
      }
   }
   // Must be in normal mode to be able to enter any other mode
   else
   {
      for (HandlerTable::const_iterator it = handlerTable_.begin(); (it != handlerTable_.end()); ++it)
      {
         ASSERT(it->second != NULL);

         if ((it->second)->CausesModeStart(input) == true)
         {
            newMode = it->first;
         }
      }
   }
  
   return newMode;
}

void Vimpc::ChangeMode(int input)
{
   Mode const oldMode = currentMode_;
   Mode const newMode = ModeAfterInput(input);

   if (newMode != oldMode)
   {
      currentMode_ = newMode;
      handlerTable_[oldMode]->FinaliseMode();
      handlerTable_[newMode]->InitialiseMode();
   }
}
