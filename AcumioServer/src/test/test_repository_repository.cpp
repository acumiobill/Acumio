//============================================================================
// Name        : test_repository_repository.cpp
// Author      : Bill Province (bill@acumio.com)
// Version     :
// Copyright   : Copyright (C) 2016 Acumio
// Description : A test_driver for the RepositoryRepository class.
//============================================================================
#include "repository_repository.h"

#include "description.pb.h"
#include "gtest_extensions.h"
#include "names.pb.h"

namespace acumio {
namespace {
void PopulateRepository(const std::string& parent_name,
                        const std::string& name,
                        model::Repository_Type type,
                        const std::string& connect_string,
                        model::Repository* repository) {
  repository->Clear();
  model::QualifiedName* qualified = repository->mutable_name();
  qualified->set_name_space(parent_name);
  qualified->set_name(name);
  repository->set_type(type);
  model::Configuration* config = repository->mutable_connect_config();
  model::Configuration_Param* param = config->add_param();
  param->set_lvalue("connect_string");
  param->set_rvalue(connect_string);
  param->set_encrypt_type(model::EncryptionType::NO_ENCRYPT);
}

void PopulateDescription(const std::string& contents,
                         const std::string& editor,
                         model::Description_SourceCategory knowledge_category,
                         const std::string& knowledge_source,
                         model::Description* description) {
  description->set_contents(contents);
  description->set_editor(editor);
  description->set_knowledge_source_category(knowledge_category);
  description->set_knowledge_source(knowledge_source);
}

void SetToRepositoryBar(model::Repository* bar) {
  PopulateRepository("", "bar", model::Repository_Type_ORACLE, "bar_connect",
                     bar);
}

void SetToRepositoryBillsHouse(model::Repository* bills_house) {
  PopulateRepository("cat", "bills_house", model::Repository_Type_ORACLE,
                     "bills_house_connect", bills_house);
}

void SetToRepositoryCat(model::Repository* cat) {
  PopulateRepository("", "cat", model::Repository_Type_ORACLE, "cat_connect",
                     cat);
}

void SetToRepositoryDog(model::Repository* dog) {
  PopulateRepository("", "dog", model::Repository_Type_ORACLE, "dog_connect",
                     dog);
}

void SetToRepositoryFoo(model::Repository* foo) {
  PopulateRepository("", "foo", model::Repository_Type_ORACLE, "foo_connect",
                     foo);
}

void SetToRepositoryInternet(model::Repository* internet) {
  PopulateRepository("cat", "internet", model::Repository_Type_ORACLE,
                     "internet_connect", internet);
}

void SetToRepositoryZebra(model::Repository* zebra) {
  PopulateRepository("", "zebra", model::Repository_Type_ORACLE,
                     "zebra_connect", zebra);
}
void SetCatDescription(model::Description* cat) {
  PopulateDescription("Repository for things related to cats",
                      "bill",
                      model::Description_SourceCategory_HUMAN_DOMAIN_KNOWLEDGE,
                      "bill's head",
                      cat);
}

void SetDogDescription(model::Description* dog) {
  PopulateDescription("Repository for things related to dogs",
                      "bill",
                      model::Description_SourceCategory_HUMAN_DOMAIN_KNOWLEDGE,
                      "bill's head",
                      dog);
}

void SetInternetDescription(model::Description* internet) {
  PopulateDescription("The Amazing and Ultimate Cat Video Database",
                      "bill",
                      model::Description_SourceCategory_HUMAN_DOMAIN_KNOWLEDGE,
                      "bill's head",
                      internet);
}

void SetBillsHouseDescription(model::Description* bills_house) {
  PopulateDescription("Repository for the cat 'Boo-Boo'",
                      "bill",
                      model::Description_SourceCategory_HUMAN_DOMAIN_KNOWLEDGE,
                      "bill's head",
                      bills_house);
}

grpc::Status CompareStringVectorToPrimaryIterator(
    std::vector<std::string> v,
    const RepositoryRepository::PrimaryIterator& begin,
    const RepositoryRepository::PrimaryIterator& end) {
  return acumio::test::CompareStringVectorToIterator<
      RepositoryRepository::PrimaryIterator>(v, begin, end);
}

grpc::Status CompareStringVectorToSecondaryIterator(
    std::vector<std::string> v,
    const RepositoryRepository::SecondaryIterator& begin,
    const RepositoryRepository::SecondaryIterator& end) {
  return acumio::test::CompareStringVectorToIterator<
      RepositoryRepository::SecondaryIterator>(v, begin, end);
}

TEST(RepositoryRepository, RepositoryConstruction) {
  RepositoryRepository repository;
  EXPECT_EQ(0, 0);
}

TEST(RepositoryRepository, AddThenRetrieve) {
  model::Repository foo;
  model::Repository bar;
  model::Repository cat;
  model::Repository dog;
  model::Repository internet;
  model::Repository bills_house;
  model::Description root_description;
  model::Description cat_description;
  model::Description dog_description;
  model::Description internet_description;
  model::Description bills_house_description;
  SetToRepositoryFoo(&foo);
  SetToRepositoryBar(&bar);
  SetToRepositoryCat(&cat);
  SetToRepositoryDog(&dog);
  SetToRepositoryInternet(&internet);
  SetToRepositoryBillsHouse(&bills_house);
  SetCatDescription(&cat_description);
  SetDogDescription(&dog_description);
  SetInternetDescription(&internet_description);
  SetBillsHouseDescription(&bills_house_description);

  RepositoryRepository repository;
  EXPECT_OK(repository.AddWithNoDescription(foo));
  EXPECT_OK(repository.AddWithNoDescription(bar));
  EXPECT_OK(repository.AddWithDescription(cat, cat_description));
  EXPECT_OK(repository.AddWithDescription(dog, dog_description));
  EXPECT_OK(repository.AddWithDescription(internet, internet_description));
  EXPECT_OK(repository.AddWithDescription(bills_house,
                                          bills_house_description));
  ASSERT_EQ(repository.size(), 6);
  model::Repository found_foo;
  model::Repository found_bar;
  model::Repository found_cat;
  model::Repository found_dog;
  model::Repository found_internet;
  model::Repository found_bills_house;
  model::Description found_foo_description;
  model::Description found_bills_house_description;
  model::QualifiedName found_bar_name;
  model::QualifiedName found_bills_house_name;
  model::QualifiedName found_cat_name;
  model::QualifiedName found_dog_name;
  model::QualifiedName found_foo_name;
  model::QualifiedName found_internet_name;
  found_bar_name.set_name_space("");
  found_bar_name.set_name("bar");
  found_bills_house_name.set_name_space("cat");
  found_bills_house_name.set_name("bills_house");
  found_cat_name.set_name_space("");
  found_cat_name.set_name("cat");
  found_dog_name.set_name_space("");
  found_dog_name.set_name("dog");
  found_foo_name.set_name_space("");
  found_foo_name.set_name("foo");
  found_internet_name.set_name_space("cat");
  found_internet_name.set_name("internet");
  
  EXPECT_OK(repository.GetRepository(found_bar_name, &found_bar));
  EXPECT_OK(repository.GetRepository(found_cat_name, &found_cat));
  EXPECT_OK(repository.GetRepository(found_dog_name, &found_dog));
  EXPECT_OK(repository.GetRepository(found_internet_name, &found_internet));
  EXPECT_OK(repository.GetRepository(found_bills_house_name,
                                     &found_bills_house));
  EXPECT_OK(repository.GetRepository(found_foo_name, &found_foo));
  EXPECT_OK(repository.GetDescription(found_foo_name, &found_foo_description));
  EXPECT_EQ(found_foo_description.contents(), "");
  EXPECT_OK(repository.GetDescription(found_bills_house_name,
                                      &found_bills_house_description));
  EXPECT_EQ(found_bills_house_description.contents(),
            "Repository for the cat 'Boo-Boo'");
}

TEST(RepositoryRepository, AddDuplicate) {
  model::Repository foo;
  RepositoryRepository repository;
  SetToRepositoryFoo(&foo);
  EXPECT_OK(repository.AddWithNoDescription(foo));
  ASSERT_EQ(repository.size(), 1);
  EXPECT_ERROR(repository.AddWithNoDescription(foo),
               grpc::StatusCode::ALREADY_EXISTS);
  ASSERT_EQ(repository.size(), 1);
}

TEST(RepositoryRepository, AddRemoveAddGet) {
  model::Repository foo;
  RepositoryRepository repository;
  SetToRepositoryFoo(&foo);
  EXPECT_OK(repository.AddWithNoDescription(foo));
  ASSERT_EQ(repository.size(), 1);

  model::QualifiedName foo_name;
  foo_name.set_name_space("");
  foo_name.set_name("foo");
  std::cout << "invoking remove operation" << std::endl;
  EXPECT_OK(repository.Remove(foo_name));
  ASSERT_EQ(repository.size(), 0);

  model::Repository found_foo;
  EXPECT_ERROR(repository.GetRepository(foo_name, &found_foo),
               grpc::StatusCode::NOT_FOUND);
}

TEST(RepositoryRepository, DeleteNotPresent) {
  model::Repository foo;
  RepositoryRepository repository;
  SetToRepositoryFoo(&foo);
  EXPECT_OK(repository.AddWithNoDescription(foo));
  ASSERT_EQ(repository.size(), 1);
  model::QualifiedName bar_qualified;
  bar_qualified.set_name_space("");
  bar_qualified.set_name("bar");
  EXPECT_ERROR(repository.Remove(bar_qualified), grpc::StatusCode::NOT_FOUND);
  ASSERT_EQ(repository.size(), 1);
}

TEST(RepositoryRepository, UpdateNoKeyChange) {
  model::Repository foo;
  model::Repository internet;
  model::Description internet_description;
  RepositoryRepository repository;
  SetToRepositoryFoo(&foo);
  SetToRepositoryInternet(&internet);
  SetInternetDescription(&internet_description);
  EXPECT_OK(repository.AddWithNoDescription(foo));
  EXPECT_OK(repository.AddWithDescription(internet, internet_description));
  ASSERT_EQ(repository.size(), 2);
  foo.mutable_connect_config()->mutable_param(0)->set_rvalue("no_connect");
  EXPECT_EQ(foo.connect_config().param(0).rvalue(), "no_connect");
  model::QualifiedName foo_qualified;
  foo_qualified.set_name_space("");
  foo_qualified.set_name("foo");
  EXPECT_OK(repository.UpdateNoDescription(foo_qualified, foo));
  ASSERT_EQ(repository.size(), 2);
  model::Repository found_foo;
  EXPECT_OK(repository.GetRepository(foo_qualified, &found_foo));
  EXPECT_EQ(found_foo.connect_config().param(0).rvalue(), "no_connect");
  internet.mutable_connect_config()->mutable_param(0)->set_rvalue("ooga booga");
  EXPECT_EQ(internet.connect_config().param(0).rvalue(), "ooga booga");
  model::QualifiedName internet_qualified;
  internet_qualified.set_name_space("cat");
  internet_qualified.set_name("internet");
  EXPECT_OK(repository.UpdateNoDescription(internet_qualified, internet));
  ASSERT_EQ(repository.size(), 2);
  model::Repository found_internet;
  EXPECT_OK(repository.GetRepository(internet_qualified, &found_internet));
  EXPECT_EQ(found_internet.connect_config().param(0).rvalue(), "ooga booga");
}

TEST(RepositoryRepository, UpdateWithKeyChange) {
  model::Repository foo;
  model::Repository bar;
  model::Description root_description;
  RepositoryRepository repository;
  model::QualifiedName bar_qualified;
  bar_qualified.set_name_space("");
  bar_qualified.set_name("bar");
  model::QualifiedName cat_qualified;
  cat_qualified.set_name_space("");
  cat_qualified.set_name("cat");
  model::QualifiedName foo_qualified;
  foo_qualified.set_name_space("");
  foo_qualified.set_name("foo");
  model::QualifiedName zebra_qualified;
  zebra_qualified.set_name_space("");
  zebra_qualified.set_name("zebra");
  SetToRepositoryFoo(&foo);
  SetToRepositoryBar(&bar);
  EXPECT_OK(repository.AddWithNoDescription(foo));
  EXPECT_OK(repository.AddWithNoDescription(bar));
  ASSERT_EQ(repository.size(), 2);
  SetToRepositoryCat(&bar);
  EXPECT_OK(repository.UpdateNoDescription(bar_qualified, bar));
  ASSERT_EQ(repository.size(), 2);
  SetToRepositoryBar(&bar);
  EXPECT_OK(repository.UpdateNoDescription(cat_qualified, bar));
  ASSERT_EQ(repository.size(), 2);
  SetToRepositoryZebra(&bar);
  EXPECT_OK(repository.UpdateNoDescription(bar_qualified, bar));
  ASSERT_EQ(repository.size(), 2);
  SetToRepositoryBar(&bar);
  EXPECT_OK(repository.UpdateNoDescription(zebra_qualified, bar));
  ASSERT_EQ(repository.size(), 2);
  SetToRepositoryCat(&foo);
  EXPECT_OK(repository.UpdateNoDescription(foo_qualified, foo));
  ASSERT_EQ(repository.size(), 2);
  SetToRepositoryFoo(&foo);
  EXPECT_OK(repository.UpdateNoDescription(cat_qualified, foo));
  ASSERT_EQ(repository.size(), 2);
  SetToRepositoryFoo(&bar);
  EXPECT_ERROR(repository.UpdateNoDescription(bar_qualified, bar),
               grpc::StatusCode::ALREADY_EXISTS);
  ASSERT_EQ(repository.size(), 2);
  SetToRepositoryBar(&foo);
  EXPECT_ERROR(repository.UpdateNoDescription(foo_qualified, foo),
               grpc::StatusCode::ALREADY_EXISTS);
  ASSERT_EQ(repository.size(), 2);
}

TEST(RepositoryRepository, DescriptionUpdates) {
  model::Repository bar;
  model::Repository foo;
  model::QualifiedName bar_qualified;
  model::QualifiedName foo_qualified;
  bar_qualified.set_name_space("");
  bar_qualified.set_name("bar");
  foo_qualified.set_name_space("");
  foo_qualified.set_name("foo");
  model::Description bar_description;
  RepositoryRepository repository;
  SetToRepositoryBar(&bar);
  SetToRepositoryFoo(&foo);
  std::string contents = "Bar Repository: required at installation";
  PopulateDescription(contents,
                      "acumio",
                      model::Description_SourceCategory_ACUMIO_INSTALLATION,
                      "installation",
                      &bar_description);
  EXPECT_OK(repository.AddWithDescription(bar, bar_description));
  EXPECT_OK(repository.AddWithNoDescription(foo));
  ASSERT_EQ(repository.size(), 2);
  model::Description found_description;
  EXPECT_OK(repository.GetDescription(bar_qualified, &found_description));
  EXPECT_EQ(found_description.contents(), contents);
  model::DescriptionHistory found_history;
  EXPECT_OK(repository.GetDescriptionHistory(bar_qualified, &found_history));
  ASSERT_EQ(found_history.version_size(), 1);
  ASSERT_EQ(found_history.version(0).contents(), contents);
  ASSERT_EQ(found_history.version(0).editor(), "acumio");
  ASSERT_EQ(found_history.version(0).knowledge_source_category(),
            model::Description_SourceCategory_ACUMIO_INSTALLATION);
  ASSERT_EQ(found_history.version(0).knowledge_source(), "installation");
  EXPECT_OK(repository.GetDescription(foo_qualified, &found_description));
  EXPECT_FALSE(found_description.has_edit_time());
  EXPECT_EQ(found_description.contents(), "");
  EXPECT_OK(repository.GetDescriptionHistory(foo_qualified, &found_history));
  EXPECT_EQ(found_history.version_size(), 0);
  bar_description.set_contents("fish");
  EXPECT_OK(repository.UpdateDescriptionOnly(bar_qualified, bar_description));
  EXPECT_OK(repository.GetDescription(bar_qualified, &found_description));
  EXPECT_EQ(found_description.contents(), "fish");
  EXPECT_OK(repository.GetDescriptionHistory(bar_qualified, &found_history));
  ASSERT_EQ(found_history.version_size(), 2);
  EXPECT_EQ(found_history.version(0).contents(), contents);
  EXPECT_EQ(found_history.version(0).editor(), "acumio");
  EXPECT_EQ(found_history.version(0).knowledge_source_category(),
            model::Description_SourceCategory_ACUMIO_INSTALLATION);
  EXPECT_EQ(found_history.version(0).knowledge_source(), "installation");
  EXPECT_EQ(found_history.version(1).contents(), "fish");
  EXPECT_EQ(found_history.version(1).editor(), "acumio");
  EXPECT_EQ(found_history.version(1).knowledge_source_category(),
            model::Description_SourceCategory_ACUMIO_INSTALLATION);
  EXPECT_EQ(found_history.version(1).knowledge_source(), "installation");
  EXPECT_OK(repository.ClearDescription(bar_qualified));
  EXPECT_OK(repository.GetDescriptionHistory(bar_qualified, &found_history));
  ASSERT_EQ(found_history.version_size(), 3);
  EXPECT_EQ(found_history.version(0).contents(), contents);
  EXPECT_EQ(found_history.version(0).editor(), "acumio");
  EXPECT_EQ(found_history.version(0).knowledge_source_category(),
            model::Description_SourceCategory_ACUMIO_INSTALLATION);
  EXPECT_EQ(found_history.version(0).knowledge_source(), "installation");
  EXPECT_EQ(found_history.version(1).contents(), "fish");
  EXPECT_EQ(found_history.version(1).editor(), "acumio");
  EXPECT_EQ(found_history.version(1).knowledge_source_category(),
            model::Description_SourceCategory_ACUMIO_INSTALLATION);
  EXPECT_EQ(found_history.version(1).knowledge_source(), "installation");
  EXPECT_EQ(found_history.version(2).contents(), "");
  EXPECT_TRUE(found_history.version(2).has_edit_time());
  EXPECT_GE(found_history.version(2).edit_time().seconds(), 0);
  // TODO: We should record properly the editor who cleared the
  // content in the APIs. This is a weakness of the  current repository
  // API that we cannot specify this in the call, but when coming from
  // the Service, this will be required once we get login credentials
  // established.
  EXPECT_EQ(found_history.version(2).editor(), "");
  EXPECT_EQ(found_history.version(2).knowledge_source_category(),
            model::Description_SourceCategory_NOT_SPECIFIED);
  EXPECT_EQ(found_history.version(2).knowledge_source(), "");

  // This should be a no-op change.
  EXPECT_OK(repository.ClearDescription(bar_qualified));
  EXPECT_OK(repository.GetDescriptionHistory(bar_qualified, &found_history));
  // And here is how we verify it: there should be no new versions of
  // the DescriptionHistory after this clearing the description twice
  // in a row.
  ASSERT_EQ(found_history.version_size(), 3);
 
  bar.mutable_connect_config()->mutable_param(0)->set_rvalue("no_connect");
  bar_description.set_contents(contents);
  EXPECT_OK(repository.UpdateWithDescription(bar_qualified,
                                             bar, bar_description));
  EXPECT_OK(repository.GetDescriptionHistory(bar_qualified, &found_history));
  ASSERT_EQ(found_history.version_size(), 4);
  EXPECT_EQ(found_history.version(3).contents(), contents);
  model::Repository found_bar;
  EXPECT_OK(repository.GetRepositoryAndDescriptionHistory(
      bar_qualified, &found_bar, &found_history));
  EXPECT_EQ(found_bar.connect_config().param(0).rvalue(), "no_connect");
  ASSERT_EQ(found_history.version_size(), 4);
  EXPECT_EQ(found_history.version(3).contents(), contents);

  bar.mutable_connect_config()->mutable_param(0)->set_rvalue("bar_connect");
  EXPECT_OK(repository.UpdateAndClearDescription(bar_qualified, bar));
  EXPECT_OK(repository.GetRepositoryAndDescriptionHistory(
      bar_qualified, &found_bar, &found_history));
  EXPECT_EQ(found_bar.connect_config().param(0).rvalue(), "bar_connect");
  ASSERT_EQ(found_history.version_size(), 5);
  EXPECT_EQ(found_history.version(4).contents(), "");
}

TEST(RepositoryRepository, IndexWalking) {
  model::Repository foo;
  model::Repository bar;
  model::Repository cat;
  model::Repository dog;
  model::Repository internet;
  model::Repository bills_house;
  model::Description cat_description;
  model::Description dog_description;
  model::Description internet_description;
  model::Description bills_house_description;
  SetToRepositoryFoo(&foo);
  SetToRepositoryBar(&bar);
  SetToRepositoryCat(&cat);
  SetToRepositoryDog(&dog);
  SetToRepositoryInternet(&internet);
  SetToRepositoryBillsHouse(&bills_house);
  SetCatDescription(&cat_description);
  SetDogDescription(&dog_description);
  SetInternetDescription(&internet_description);
  SetBillsHouseDescription(&bills_house_description);

  RepositoryRepository repository;
  EXPECT_OK(repository.AddWithNoDescription(foo));
  EXPECT_OK(repository.AddWithNoDescription(bar));
  EXPECT_OK(repository.AddWithDescription(cat, cat_description));
  EXPECT_OK(repository.AddWithDescription(dog, dog_description));
  EXPECT_OK(repository.AddWithDescription(internet, internet_description));
  EXPECT_OK(repository.AddWithDescription(bills_house,
                                          bills_house_description));
  ASSERT_EQ(repository.size(), 6);
  std::vector<std::string> primary_order;
  primary_order.push_back(" bar");
  primary_order.push_back(" cat");
  primary_order.push_back(" dog");
  primary_order.push_back(" foo");
  primary_order.push_back("cat bills_house");
  primary_order.push_back("cat internet");
  ASSERT_EQ(primary_order.size(), 6);
  EXPECT_OK(CompareStringVectorToPrimaryIterator(primary_order,
                                                 repository.primary_begin(),
                                                 repository.primary_end()));
  primary_order.erase(primary_order.begin(), primary_order.begin()+1);
  ASSERT_EQ(primary_order.size(), 5);
  model::QualifiedName full_name_c;
  model::QualifiedName full_name_cat;
  full_name_c.set_name_space("");
  full_name_c.set_name("c");
  full_name_cat.set_name_space("");
  full_name_cat.set_name("cat");
  EXPECT_OK(CompareStringVectorToPrimaryIterator(
      primary_order,
      repository.LowerBoundByFullName(full_name_c),
      repository.primary_end()));
  EXPECT_OK(CompareStringVectorToPrimaryIterator(
      primary_order,
      repository.LowerBoundByFullName(full_name_cat),
      repository.primary_end()));

  std::vector<std::string> secondary_order;
  secondary_order.push_back("");
  secondary_order.push_back("");
  secondary_order.push_back("");
  secondary_order.push_back("");
  secondary_order.push_back("cat");
  secondary_order.push_back("cat");
  ASSERT_EQ(secondary_order.size(), 6);
  EXPECT_OK(CompareStringVectorToSecondaryIterator(
      secondary_order,
      repository.namespace_begin(),
      repository.namespace_end()));
  secondary_order.erase(secondary_order.begin(), secondary_order.begin()+4);
  ASSERT_EQ(secondary_order.size(), 2);
  EXPECT_OK(CompareStringVectorToSecondaryIterator(
      secondary_order,
      repository.LowerBoundByNamespace("a"),
      repository.namespace_end()));
}

} // anonymous namespace
} // namespace acumio

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
