<<<<<<< HEAD
=======
<<<<<<< HEAD
>>>>>>> e60a744b5de510d10c094176631bfe9fca952d18
#include "form.h"
#include "ui_form.h"

Form::Form(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Form)
{
    ui->setupUi(this);
}

Form::~Form()
{
    delete ui;
}
<<<<<<< HEAD
=======
=======
#include "form.h"
#include "ui_form.h"

Form::Form(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Form)
{
    ui->setupUi(this);
}

Form::~Form()
{
    delete ui;
}
>>>>>>> 424bc0a8b89776bc4a6d5328940fb4156ce50bcf
>>>>>>> e60a744b5de510d10c094176631bfe9fca952d18
