#include "v800main.h"
#include "ui_v800main.h"
#include "v800usb.h"
#include "v800fs.h"

#include <QtWidgets>

V800Main::V800Main(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::V800Main)
{
    qRegisterMetaType<QList<QString> >("QList<QString>");

    v800_ready = false;

    QThread *usb_thread = new QThread;
    usb = new V800usb();
    usb->moveToThread(usb_thread);

    connect(usb, SIGNAL(all_sessions(QList<QString>)), this, SLOT(handle_all_sessions(QList<QString>)));
    connect(usb, SIGNAL(sessions_done()), this, SLOT(handle_sessions_done()));
    connect(usb, SIGNAL(session_done()), this, SLOT(handle_session_done()));
    connect(usb, SIGNAL(ready()), this, SLOT(handle_ready()));
    connect(usb, SIGNAL(not_ready()), this, SLOT(handle_not_ready()));

    connect(this, SIGNAL(get_sessions(QList<QString>, QString, bool)), usb, SLOT(get_sessions(QList<QString>, QString, bool)));

    new QShortcut(QKeySequence(Qt::SHIFT + Qt::Key_A), this, SLOT(handle_advanced_shortcut()));

    connect(usb_thread, SIGNAL(started()), usb, SLOT(start()));
    usb_thread->start();

    ui->setupUi(this);
    ui->verticalLayout->setAlignment(Qt::AlignTop);
    ui->verticalLayout->setSpacing(20);
    ui->exerciseTree->setColumnCount(1);
    ui->exerciseTree->setHeaderLabel(tr("Session"));

    ui->fsBtn->setVisible(false);
    ui->rawChk->setVisible(false);

    disable_all();
    this->show();

    start_in_progress = new QMessageBox(this);
    start_in_progress->setWindowTitle(tr("V800 Downloader"));
    start_in_progress->setText(tr("Downloading session list from V800..."));
    start_in_progress->setIcon(QMessageBox::Information);
    start_in_progress->setStandardButtons(0);
    start_in_progress->setWindowModality(Qt::WindowModal);
    start_in_progress->exec();
}

V800Main::~V800Main()
{
    delete ui;
    delete usb;
}

void V800Main::handle_not_ready()
{
    QMessageBox failure;
    failure.setWindowTitle(tr("V800 Downloader"));
    failure.setText(tr("Failed to open V800!"));
    failure.setIcon(QMessageBox::Critical);
    failure.exec();

    exit(-1);
}

void V800Main::handle_ready()
{
    start_in_progress->done(0);

    v800_ready = true;
    enable_all();
}

void V800Main::handle_all_sessions(QList<QString> sessions)
{
    QList<QTreeWidgetItem *> items;

    for(int sessions_iter = 0; sessions_iter < sessions.length(); sessions_iter++)
    {
        QTreeWidgetItem *item = new QTreeWidgetItem((QTreeWidget *)0, QStringList(sessions[sessions_iter]));
        item->setCheckState(0, Qt::Unchecked);
        items.append(item);
    }
    ui->exerciseTree->insertTopLevelItems(0, items);
}

void V800Main::handle_session_done()
{
    int cur_session;

    cur_session = download_progress->value();
    download_progress->setValue(cur_session+1);
    download_progress->setLabelText(tr("Downloading %1/%2...").arg(cur_session+1).arg(sessions_cnt));
}

void V800Main::handle_sessions_done()
{
    download_progress->done(0);
    QMessageBox done;
    done.setWindowTitle(tr("V800 Downloader"));
    done.setText(tr("Done downloading sessions! Run Bipolar to convert them to exportable formats."));
    done.setIcon(QMessageBox::Information);
    done.setWindowModality(Qt::WindowModal);
    done.exec();

    enable_all();
}

void V800Main::handle_advanced_shortcut()
{
    if(ui->fsBtn->isVisible())
        ui->fsBtn->setVisible(false);
    else
        ui->fsBtn->setVisible(true);

    if(ui->rawChk->isVisible())
        ui->rawChk->setVisible(false);
    else
        ui->rawChk->setVisible(true);
}

void V800Main::enable_all()
{
    ui->downloadBtn->setEnabled(true);
    ui->rawChk->setEnabled(true);
    ui->checkBtn->setEnabled(true);
    ui->uncheckBtn->setEnabled(true);
    ui->fsBtn->setEnabled(true);
}

void V800Main::disable_all()
{
    ui->downloadBtn->setEnabled(false);
    ui->rawChk->setEnabled(false);
    ui->checkBtn->setEnabled(false);
    ui->uncheckBtn->setEnabled(false);
    ui->fsBtn->setEnabled(false);
}

void V800Main::on_downloadBtn_clicked()
{
    QList<QString> sessions;
    QString save_dir;
    int item_iter;

    disable_all();
    sessions_cnt = 0;

    if(ui->rawChk->isChecked())
    {
        save_dir = QFileDialog::getExistingDirectory(this, tr("Open Directory"), QDir::homePath(), QFileDialog::ShowDirsOnly);
        if(save_dir == tr(""))
        {
            enable_all();
            return;
        }
    }

    for(item_iter = 0; item_iter < ui->exerciseTree->topLevelItemCount(); item_iter++)
    {
        if(ui->exerciseTree->topLevelItem(item_iter)->checkState(0) == Qt::Checked)
        {
            sessions_cnt++;
            sessions.append(ui->exerciseTree->topLevelItem(item_iter)->text(0));
        }
    }

    download_progress = new QProgressDialog(tr("Downloading 1/%1...").arg(sessions_cnt), tr("Cancel"), 0, sessions_cnt+1, this);
    download_progress->setCancelButton(0);
    download_progress->setWindowModality(Qt::WindowModal);
    download_progress->setValue(1);
    download_progress->setWindowTitle(tr("V800 Downloader"));
    download_progress->show();

    emit get_sessions(sessions, save_dir, ui->rawChk->isChecked());
}

void V800Main::on_checkBtn_clicked()
{
    int item_iter;

    for(item_iter = 0; item_iter < ui->exerciseTree->topLevelItemCount(); item_iter++)
        ui->exerciseTree->topLevelItem(item_iter)->setCheckState(0, Qt::Checked);
}

void V800Main::on_uncheckBtn_clicked()
{
    int item_iter;

    for(item_iter = 0; item_iter < ui->exerciseTree->topLevelItemCount(); item_iter++)
        ui->exerciseTree->topLevelItem(item_iter)->setCheckState(0, Qt::Unchecked);
}

void V800Main::on_fsBtn_clicked()
{
    V800fs *fs = new V800fs(usb);
    fs->setWindowModality(Qt::WindowModal);
    fs->show();
}
