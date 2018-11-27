/*
 *  Copyright (C) 2013 Felix Geyer <debfx@fobos.de>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 or (at your option)
 *  version 3 of the License.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef KEEPASSX_MESSAGEBOX_H
#define KEEPASSX_MESSAGEBOX_H

#include <QMessageBox>
#include <QPushButton>
#include <QMap>

class MessageBox
{
public:
    enum Button : uint32_t {
        // Reimplementation of Qt StandardButtons
        NoButton        = 0,
        Ok              = 1 << 1,
        Open            = 1 << 2,
        Save            = 1 << 3,
        Cancel          = 1 << 4,
        Close           = 1 << 5,
        Discard         = 1 << 6,
        Apply           = 1 << 7,
        Reset           = 1 << 8,
        RestoreDefaults = 1 << 9,
        Help            = 1 << 10,
        SaveAll         = 1 << 11,
        Yes             = 1 << 12,
        YesToAll        = 1 << 13,
        No              = 1 << 14,
        NoToAll         = 1 << 15,
        Abort           = 1 << 16,
        Retry           = 1 << 17,
        Ignore          = 1 << 18,

        // KeePassXC Buttons
        Overwrite       = 1 << 19,
        Delete          = 1 << 20,

        // Internal loop markers. Update Last when new KeePassXC button is added
        First = Ok,
        Last = Delete,
    };

    typedef uint32_t Buttons;

    static QMessageBox::StandardButton critical(QWidget* parent,
                                                const QString& title,
                                                const QString& text,
                                                QMessageBox::StandardButtons buttons = QMessageBox::Ok,
                                                QMessageBox::StandardButton defaultButton = QMessageBox::NoButton);
    static QMessageBox::StandardButton information(QWidget* parent,
                                                   const QString& title,
                                                   const QString& text,
                                                   QMessageBox::StandardButtons buttons = QMessageBox::Ok,
                                                   QMessageBox::StandardButton defaultButton = QMessageBox::NoButton);
    static QMessageBox::StandardButton question(QWidget* parent,
                                                const QString& title,
                                                const QString& text,
                                                QMessageBox::StandardButtons buttons = QMessageBox::Ok,
                                                QMessageBox::StandardButton defaultButton = QMessageBox::NoButton);
    static MessageBox::Button newQuestion(QWidget* parent,
                                        const QString& title,
                                        const QString& text,
                                        MessageBox::Buttons buttons = MessageBox::Ok,
                                        MessageBox::Button defaultButton = MessageBox::NoButton);
    static QMessageBox::StandardButton warning(QWidget* parent,
                                               const QString& title,
                                               const QString& text,
                                               QMessageBox::StandardButtons buttons = QMessageBox::Ok,
                                               QMessageBox::StandardButton defaultButton = QMessageBox::NoButton);

    static void setNextAnswer(QMessageBox::StandardButton button);
    static void setNewNextAnswer(MessageBox::Button button);

private:
    static QMessageBox::StandardButton m_nextAnswer;
    static Button m_newNextAnswer;

    static void addButton(QMessageBox &box, MessageBox::Button button);

    static QMap<QAbstractButton*, Button> m_buttonMap;
};

#endif // KEEPASSX_MESSAGEBOX_H
