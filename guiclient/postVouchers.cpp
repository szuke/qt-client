/*
 * This file is part of the xTuple ERP: PostBooks Edition, a free and
 * open source Enterprise Resource Planning software suite,
 * Copyright (c) 1999-2014 by OpenMFG LLC, d/b/a xTuple.
 * It is licensed to you under the Common Public Attribution License
 * version 1.0, the full text of which (including xTuple-specific Exhibits)
 * is available at www.xtuple.com/CPAL.  By using this software, you agree
 * to be bound by its terms.
 */

#include "postVouchers.h"

#include <QMessageBox>
#include "guiErrorCheck.h"
#include <QSqlError>

#include <openreports.h>
#include "errorReporter.h"

postVouchers::postVouchers(QWidget* parent, const char* name, bool modal, Qt::WindowFlags fl)
    : XDialog(parent, name, modal, fl)
{
  setupUi(this);

  connect(_post, SIGNAL(clicked()), this, SLOT(sPost()));

  if (_preferences->boolean("XCheckBox/forgetful"))
    _printJournal->setChecked(true);
}

postVouchers::~postVouchers()
{
  // no need to delete child widgets, Qt does it all for us
}

void postVouchers::languageChange()
{
    retranslateUi(this);
}

void postVouchers::sPost()
{
  XSqlQuery postPost;
  postPost.prepare("SELECT count(*) AS unposted FROM vohead WHERE (NOT vohead_posted)");
  postPost.exec();

  QList<GuiErrorCheck> errors;
  errors<< GuiErrorCheck(postPost.first() && postPost.value("unposted").toInt()==0, _post,
                         tr("There are no Vouchers to post."))
  ;
  if (GuiErrorCheck::reportErrors(this, tr("No Vouchers to Post"), errors))
    return;

  postPost.prepare("SELECT postVouchers(false) AS result;");
  postPost.exec();
  if (postPost.first())
  {
    int result = postPost.value("result").toInt();
    if (result == -5)
    {
      QMessageBox::critical( this, tr("Cannot Post Voucher"),
                             tr( "The Cost Category(s) for one or more Item Sites for the Purchase Order covered by one or more\n"
                                 "of the Vouchers that you are trying to post is not configured with Purchase Price Variance or\n"
                                 "P/O Liability Clearing Account Numbers or the Vendor of these Vouchers is not configured with an\n"
                                 "A/P Account Number.  Because of this, G/L Transactions cannot be posted for these Vouchers.\n"
                                 "You must contact your Systems Administrator to have this corrected before you may\n"
                                 "post Vouchers." ) );
      return;
    }
    else if (result < 0)
    {
      ErrorReporter::error(QtCriticalMsg, this, tr("Error Posting Voucher"),
                           postPost, __FILE__, __LINE__);
      return;
    }

    omfgThis->sVouchersUpdated();
 
    if (_printJournal->isChecked())
    {
      ParameterList params;
      params.append("source", "A/P");
      params.append("sourceLit", tr("A/P"));
      params.append("startJrnlnum", result);
      params.append("endJrnlnum", result);

      if (_metrics->boolean("UseJournals"))
      {
        params.append("title",tr("Journal Series"));
        params.append("table", "sltrans");
      }
      else
      {
        params.append("title",tr("General Ledger Series"));
        params.append("gltrans", true);
        params.append("table", "gltrans");
      }

      orReport report("GLSeries", params);
      if (report.isValid())
        report.print();
      else
        report.reportError(this);
    }
  }
  else
  {
    ErrorReporter::error(QtCriticalMsg, this, tr("Error Posting Voucher"),
                         postPost, __FILE__, __LINE__);
    return;
  }

  accept();
}
