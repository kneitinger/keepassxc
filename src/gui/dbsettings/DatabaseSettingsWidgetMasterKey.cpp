/*
 *  Copyright (C) 2018 KeePassXC Team <team@keepassxc.org>
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

#include "DatabaseSettingsWidgetMasterKey.h"
#include "core/Database.h"
#include "keys/PasswordKey.h"
#include "keys/FileKey.h"
#include "keys/YkChallengeResponseKey.h"
#include "gui/MessageBox.h"
#include "gui/masterkey/PasswordEditWidget.h"
#include "gui/masterkey/KeyFileEditWidget.h"
#include "gui/masterkey/YubiKeyEditWidget.h"

#include <QVBoxLayout>
#include <QSpacerItem>
#include <QPushButton>

DatabaseSettingsWidgetMasterKey::DatabaseSettingsWidgetMasterKey(QWidget* parent)
    : DatabaseSettingsWidget(parent)
    , m_additionalKeyOptionsToggle(new QPushButton(tr("Add additional protection..."), this))
    , m_additionalKeyOptions(new QWidget(this))
    , m_passwordEditWidget(new PasswordEditWidget(this))
    , m_keyFileEditWidget(new KeyFileEditWidget(this))
#ifdef WITH_XC_YUBIKEY
    , m_yubiKeyEditWidget(new YubiKeyEditWidget(this))
#endif
{
    auto* vbox = new QVBoxLayout(this);
    vbox->setSizeConstraint(QLayout::SetMinimumSize);

    // primary password option
    vbox->addWidget(m_passwordEditWidget);

    // additional key options
    m_additionalKeyOptionsToggle->setObjectName("additionalKeyOptionsToggle");
    vbox->addWidget(m_additionalKeyOptionsToggle);
    vbox->addWidget(m_additionalKeyOptions);
    vbox->setSizeConstraint(QLayout::SetMinimumSize);
    m_additionalKeyOptions->setLayout(new QVBoxLayout());
    m_additionalKeyOptions->layout()->setMargin(0);
    m_additionalKeyOptions->layout()->addWidget(m_keyFileEditWidget);
#ifdef WITH_XC_YUBIKEY
    m_additionalKeyOptions->layout()->addWidget(m_yubiKeyEditWidget);
#endif
    m_additionalKeyOptions->setVisible(false);

    connect(m_additionalKeyOptionsToggle, SIGNAL(clicked()), SLOT(showAdditionalKeyOptions()));

    vbox->addStretch();
    setLayout(vbox);
}

DatabaseSettingsWidgetMasterKey::~DatabaseSettingsWidgetMasterKey()
{
}

void DatabaseSettingsWidgetMasterKey::load(QSharedPointer<Database> db)
{
    DatabaseSettingsWidget::load(db);

    if (!m_db->key() || m_db->key()->keys().isEmpty()) {
        // database has no key, we are about to add a new one
        m_passwordEditWidget->changeVisiblePage(KeyComponentWidget::Page::Edit);
        m_passwordEditWidget->setPasswordVisible(true);
        m_isDirty = true;
        return;
    }

    bool isDirty = false;
    bool hasAdditionalKeys = false;
    for (const auto& key: m_db->key()->keys()) {
        if (key->uuid() == PasswordKey::UUID) {
            m_passwordEditWidget->setComponentAdded(true);
        } else if (key->uuid() == FileKey::UUID) {
            m_keyFileEditWidget->setComponentAdded(true);
            hasAdditionalKeys = true;
        }
    }

#ifdef WITH_XC_YUBIKEY
    for (const auto& key: m_db->key()->challengeResponseKeys()) {
        if (key->uuid() == YkChallengeResponseKey::UUID) {
            m_yubiKeyEditWidget->setComponentAdded(true);
            hasAdditionalKeys = true;
        }
    }
#endif

    setAdditionalKeyOptionsVisible(hasAdditionalKeys);

    m_isDirty = isDirty;
}

void DatabaseSettingsWidgetMasterKey::initialize()
{
    bool blocked = blockSignals(true);
    m_passwordEditWidget->setComponentAdded(false);
    m_keyFileEditWidget->setComponentAdded(false);
#ifdef WITH_XC_YUBIKEY
    m_yubiKeyEditWidget->setComponentAdded(false);
#endif
    blockSignals(blocked);
}

void DatabaseSettingsWidgetMasterKey::uninitialize()
{
}

bool DatabaseSettingsWidgetMasterKey::save()
{
    m_isDirty |= (m_passwordEditWidget->visiblePage() == KeyComponentWidget::Page::Edit);
    m_isDirty |= (m_keyFileEditWidget->visiblePage() == KeyComponentWidget::Page::Edit);
#ifdef WITH_XC_YUBIKEY
    m_isDirty |= (m_yubiKeyEditWidget->visiblePage() == KeyComponentWidget::Page::Edit);
#endif

    if (m_db->key() && ! m_db->key()->keys().isEmpty() && !m_isDirty) {
        // key unchanged
        return true;
    }

    auto newKey = QSharedPointer<CompositeKey>::create();

    QSharedPointer<Key> passwordKey;
    QSharedPointer<Key> fileKey;
    QSharedPointer<ChallengeResponseKey> ykCrKey;

    for (const auto& key: m_db->key()->keys()) {
        if (key->uuid() == PasswordKey::UUID) {
            passwordKey = key;
        } else if (key->uuid() == FileKey::UUID) {
            fileKey = key;
        }
    }

    for (const auto& key: m_db->key()->challengeResponseKeys()) {
        if (key->uuid() == YkChallengeResponseKey::UUID) {
            ykCrKey = key;
        }
    }

    if (!addToCompositeKey(m_passwordEditWidget, newKey, passwordKey)) {
        return false;
    }

    if (!addToCompositeKey(m_keyFileEditWidget, newKey, fileKey)) {
        return false;
    }

#ifdef WITH_XC_YUBIKEY
    if (!addToCompositeKey(m_yubiKeyEditWidget, newKey, ykCrKey)) {
        return false;
    }
#endif

    if (newKey->keys().isEmpty() && newKey->challengeResponseKeys().isEmpty()) {
        MessageBox::critical(this, tr("No encryption key added"),
                             tr("You must add at least one encryption key to secure your database!"),
                             MessageBox::Ok, MessageBox::Ok);
        return false;
    }

    if (m_passwordEditWidget->visiblePage() == KeyComponentWidget::AddNew) {
        auto answer = MessageBox::warning(this, tr("No password set"),
                                          tr("WARNING! You have not set a password. Using a database without "
                                             "a password is strongly discouraged!\n\n"
                                             "Are you sure you want to continue without a password?"),
                                          MessageBox::Yes | MessageBox::Cancel, MessageBox::Cancel);
        if (answer != MessageBox::Yes) {
            return false;
        }
    }

    m_db->setKey(newKey);

    emit editFinished(true);
    return true;
}

void DatabaseSettingsWidgetMasterKey::discard()
{
    emit editFinished(false);
}

void DatabaseSettingsWidgetMasterKey::showAdditionalKeyOptions()
{
    setAdditionalKeyOptionsVisible(true);
}

void DatabaseSettingsWidgetMasterKey::setAdditionalKeyOptionsVisible(bool show)
{
    m_additionalKeyOptionsToggle->setVisible(!show);
    m_additionalKeyOptions->setVisible(show);
    m_additionalKeyOptions->layout()->setSizeConstraint(QLayout::SetMinimumSize);
    emit sizeChanged();
}

bool DatabaseSettingsWidgetMasterKey::addToCompositeKey(KeyComponentWidget* widget,
    QSharedPointer<CompositeKey>& newKey, QSharedPointer<Key>& oldKey)
{
    if (widget->visiblePage() == KeyComponentWidget::Edit) {
        QString error = tr("Unknown error");
        if (!widget->validate(error) || !widget->addToCompositeKey(newKey)) {
            MessageBox::critical(this, tr("Failed to change master key"), error, MessageBox::Ok);
            return false;
        }
    } else if (widget->visiblePage() == KeyComponentWidget::LeaveOrRemove) {
        Q_ASSERT(oldKey);
        newKey->addKey(oldKey);
    }
    return true;
}

bool DatabaseSettingsWidgetMasterKey::addToCompositeKey(KeyComponentWidget* widget,
    QSharedPointer<CompositeKey>& newKey, QSharedPointer<ChallengeResponseKey>& oldKey)
{
    if (widget->visiblePage() == KeyComponentWidget::Edit) {
        QString error = tr("Unknown error");
        if (!widget->validate(error) || !widget->addToCompositeKey(newKey)) {
            MessageBox::critical(this, tr("Failed to change master key"), error, MessageBox::Ok);
            return false;
        }
    } else if (widget->visiblePage() == KeyComponentWidget::LeaveOrRemove) {
        Q_ASSERT(oldKey);
        newKey->addChallengeResponseKey(oldKey);
    }
    return true;
}

void DatabaseSettingsWidgetMasterKey::markDirty()
{
    m_isDirty = true;
}
