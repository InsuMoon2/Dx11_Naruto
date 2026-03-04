#include "pch.h"
#include "CommandHistory.h"

#include "ICommand.h"
#include "Notification_Manager.h"

void CommandHistory::Execute(Shared<ICommand> cmd)
{
    cmd->Execute();
    _undoStack.push_back(cmd);
    _redoStack.clear();     // 새 커멘드 실행 시 redo 초기화

    // 용량 제한. 필요한가?
    if (_undoStack.size() > MAX_HISTORY)
        _undoStack.erase(_undoStack.begin());

}

void CommandHistory::Undo()
{
    if (_undoStack.empty()) return;

    auto cmd = _undoStack.back();
    _undoStack.pop_back();
    cmd->Undo();
    _redoStack.push_back(cmd);

    EDITOR->Get_Notification()->Add_Notification_With_Type(
        ENotifyType::Info, "Undo : {}", cmd->Get_Description());
}

void CommandHistory::Redo()
{
    if (_redoStack.empty()) return;

    auto cmd = _redoStack.back();
    _redoStack.pop_back();
    cmd->Redo();
    _undoStack.push_back(cmd);

    EDITOR->Get_Notification()->Add_Notification_With_Type(
        ENotifyType::Info, "Redo: {}", cmd->Get_Description());
}

void CommandHistory::Clear()
{
    _undoStack.clear();
    _redoStack.clear();
}

Unique<CommandHistory> CommandHistory::Create()
{
    return make_unique<CommandHistory>();
}
