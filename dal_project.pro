TEMPLATE = subdirs

SUBDIRS += DAL DAL_test

DAL.file = sources/dal.pro
DAL_test.file = test_app/dal_test.pro

DAL_test.depends = DAL

