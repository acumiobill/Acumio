//============================================================================
// Name        : test_namespace_repository.cpp
// Author      : Bill Province (bill@acumio.com)
// Version     :
// Copyright   : Copyright (C) 2016 Acumio
// Description : A test_driver for the NamespaceRepository class.
//============================================================================
#include "namespace_repository.h"

#include "description.pb.h"
#include "gtest_extensions.h"
#include "names.pb.h"

namespace acumio {
namespace {
void PopulateFullName(const std::string& parent_name,
                      const std::string& separator,
                      const std::string& short_name,
                      std::string* fullname) {
  fullname->clear();
  fullname->reserve(parent_name.length() +
                    separator.length() +
                    short_name.length());
  fullname->append(parent_name).append(separator).append(short_name);
}

void PopulateNamespace(const model::Namespace& parent,
                       const std::string& name,
                       const std::string& separator,
                       bool is_repository_name,
                       model::Namespace* name_space) {
  name_space->Clear();
  PopulateFullName(parent.full_name(), parent.separator(), name,
                   name_space->mutable_full_name());
  model::QualifiedName* namespace_name = name_space->mutable_name();
  namespace_name->set_name_space(parent.full_name());
  namespace_name->set_name(name);
  name_space->set_separator(separator);
  name_space->set_is_repository_name(is_repository_name);
}

void PopulateRootNamespace(model::Namespace* name_space) {
  model::QualifiedName* qualified = name_space->mutable_name();
  qualified->set_name("");
  qualified->set_name_space("");
  name_space->set_separator("");
  name_space->set_is_repository_name(false);
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

void SetToNamespaceFoo(const model::Namespace& root_namespace,
                       model::Namespace* foo) {
  PopulateNamespace(root_namespace, "foo", "::", false, foo);
}

void SetToNamespaceBar(const model::Namespace& root_namespace,
                       model::Namespace* bar) {
  PopulateNamespace(root_namespace, "bar", "::", false, bar);
}

void SetToNamespaceCat(const model::Namespace& root_namespace,
                       model::Namespace* cat) {
  PopulateNamespace(root_namespace, "cat", "::", false, cat);
}

void SetToNamespaceDog(const model::Namespace& root_namespace,
                       model::Namespace* dog) {
  PopulateNamespace(root_namespace, "dog", "::", false, dog);
}

void SetToNamespaceZebra(const model::Namespace& root_namespace,
                       model::Namespace* zebra) {
  PopulateNamespace(root_namespace, "zebra", "::", false, zebra);
}

grpc::Status CompareStringVectorToPrimaryIterator(
    std::vector<std::string> v,
    const NamespaceRepository::PrimaryIterator& begin,
    const NamespaceRepository::PrimaryIterator& end) {
  return acumio::test::CompareStringVectorToIterator<
      NamespaceRepository::PrimaryIterator>(v, begin, end);
}

grpc::Status CompareStringVectorToSecondaryIterator(
    std::vector<std::string> v,
    const NamespaceRepository::SecondaryIterator& begin,
    const NamespaceRepository::SecondaryIterator& end) {
  return acumio::test::CompareStringVectorToIterator<
      NamespaceRepository::SecondaryIterator>(v, begin, end);
}

TEST(NamespaceRepository, RepositoryConstruction) {
  NamespaceRepository repository;
  EXPECT_EQ(0, 0);
}

TEST(NamespaceRepository, AddThenRetrieve) {
  model::Namespace root;
  model::Namespace foo;
  model::Namespace bar;
  model::Namespace cat;
  model::Namespace dog;
  model::Namespace internet;
  model::Namespace bills_house;
  model::Description root_description;
  model::Description cat_description;
  model::Description dog_description;
  model::Description internet_description;
  model::Description bills_house_description;
  PopulateRootNamespace(&root);
  SetToNamespaceFoo(root, &foo);
  SetToNamespaceBar(root, &bar);
  SetToNamespaceCat(root, &cat);
  SetToNamespaceDog(root, &dog);
  PopulateNamespace(cat, "internet", "/", true, &internet);
  PopulateNamespace(cat, "bills_house", "/", true, &bills_house);
  PopulateDescription("root Namespace: required at installation",
                      "acumio",
                      model::Description_SourceCategory_ACUMIO_INSTALLATION,
                      "installation",
                      &root_description);
  PopulateDescription("Namespace for things related to cats",
                      "bill",
                      model::Description_SourceCategory_HUMAN_DOMAIN_KNOWLEDGE,
                      "bill's head",
                      &cat_description);
  PopulateDescription("Namespace for things related to dogs",
                      "bill",
                      model::Description_SourceCategory_HUMAN_DOMAIN_KNOWLEDGE,
                      "bill's head",
                      &dog_description);
  PopulateDescription("The Amazing and Ultimate Cat Video Database",
                      "bill",
                      model::Description_SourceCategory_HUMAN_DOMAIN_KNOWLEDGE,
                      "bill's head",
                      &internet_description);
  PopulateDescription("Repository for the cat 'Boo-Boo'",
                      "bill",
                      model::Description_SourceCategory_HUMAN_DOMAIN_KNOWLEDGE,
                      "bill's head",
                      &bills_house_description);

  NamespaceRepository repository;
  EXPECT_OK(repository.AddWithDescription(root, root_description));
  EXPECT_OK(repository.AddWithNoDescription(foo));
  EXPECT_OK(repository.AddWithNoDescription(bar));
  EXPECT_OK(repository.AddWithDescription(cat, cat_description));
  EXPECT_OK(repository.AddWithDescription(dog, dog_description));
  EXPECT_OK(repository.AddWithDescription(internet, internet_description));
  EXPECT_OK(repository.AddWithDescription(bills_house,
                                          bills_house_description));
  ASSERT_EQ(repository.size(), 7);
  model::Namespace found_root;
  model::Namespace found_foo;
  model::Namespace found_bar;
  model::Namespace found_cat;
  model::Namespace found_dog;
  model::Namespace found_internet;
  model::Namespace found_bills_house;
  model::Description found_root_description;
  model::Description second_found_root_description;
  model::Description found_foo_description;
  model::Description second_found_foo_description;

  EXPECT_OK(repository.GetNamespace("", &found_root));
  EXPECT_OK(repository.GetNamespace("foo", &found_foo));
  EXPECT_OK(repository.GetNamespace("bar", &found_bar));
  EXPECT_OK(repository.GetNamespace("cat", &found_cat));
  EXPECT_OK(repository.GetNamespace("dog", &found_dog));
  EXPECT_OK(repository.GetNamespace("cat::internet", &found_internet));
  EXPECT_OK(repository.GetNamespace("cat::bills_house", &found_bills_house));
  EXPECT_OK(repository.GetDescription("", &found_root_description));
  EXPECT_EQ(found_root_description.contents(), root_description.contents());
  EXPECT_OK(repository.GetDescription("foo", &found_foo_description));
  EXPECT_EQ(found_foo_description.contents(), "");
  EXPECT_GT(found_root_description.edit_time().seconds(), 0);
  EXPECT_OK(
      repository.GetNamespaceAndDescription("",
                                            &found_root,
                                            &second_found_root_description));
  EXPECT_EQ(second_found_root_description.contents(),
            root_description.contents());
  EXPECT_EQ(found_root_description.edit_time().seconds(),
            second_found_root_description.edit_time().seconds());
  EXPECT_EQ(found_root_description.edit_time().nanos(),
            second_found_root_description.edit_time().nanos());
}

TEST(NamespaceRepository, AddDuplicate) {
  model::Namespace root;
  model::Namespace foo;
  model::Description root_description;
  NamespaceRepository repository;
  PopulateRootNamespace(&root);
  SetToNamespaceFoo(root, &foo);
  PopulateDescription("root Namespace: required at installation",
                      "acumio",
                      model::Description_SourceCategory_ACUMIO_INSTALLATION,
                      "installation",
                      &root_description);
  EXPECT_OK(repository.AddWithDescription(root, root_description));
  EXPECT_OK(repository.AddWithNoDescription(foo));
  ASSERT_EQ(repository.size(), 2);
  EXPECT_ERROR(repository.AddWithDescription(root, root_description),
               grpc::StatusCode::ALREADY_EXISTS);
  ASSERT_EQ(repository.size(), 2);
  EXPECT_ERROR(repository.AddWithNoDescription(foo),
               grpc::StatusCode::ALREADY_EXISTS);
  ASSERT_EQ(repository.size(), 2);
}

TEST(NamespaceRepository, AddRemoveAddGet) {
  model::Namespace root;
  model::Namespace foo;
  model::Description root_description;
  NamespaceRepository repository;
  PopulateRootNamespace(&root);
  SetToNamespaceFoo(root, &foo);
  PopulateDescription("root Namespace: required at installation",
                      "acumio",
                      model::Description_SourceCategory_ACUMIO_INSTALLATION,
                      "installation",
                      &root_description);
  EXPECT_OK(repository.AddWithDescription(root, root_description));
  EXPECT_OK(repository.AddWithNoDescription(foo));
  ASSERT_EQ(repository.size(), 2);

  EXPECT_OK(repository.Remove("foo"));
  ASSERT_EQ(repository.size(), 1);

  model::Namespace found_foo;
  EXPECT_ERROR(repository.GetNamespace("foo", &found_foo),
               grpc::StatusCode::NOT_FOUND);
}

TEST(NamespaceRepository, DeleteNotPresent) {
  model::Namespace root;
  model::Namespace foo;
  model::Description root_description;
  NamespaceRepository repository;
  PopulateRootNamespace(&root);
  SetToNamespaceFoo(root, &foo);
  PopulateDescription("root Namespace: required at installation",
                      "acumio",
                      model::Description_SourceCategory_ACUMIO_INSTALLATION,
                      "installation",
                      &root_description);
  EXPECT_OK(repository.AddWithDescription(root, root_description));
  EXPECT_OK(repository.AddWithNoDescription(foo));
  ASSERT_EQ(repository.size(), 2);
  EXPECT_ERROR(repository.Remove("bar"), grpc::StatusCode::NOT_FOUND);
  ASSERT_EQ(repository.size(), 2);
}

TEST(NamespaceRepository, UpdateNoKeyChange) {
  model::Namespace root;
  model::Namespace foo;
  model::Description root_description;
  NamespaceRepository repository;
  PopulateRootNamespace(&root);
  SetToNamespaceFoo(root, &foo);
  PopulateDescription("root Namespace: required at installation",
                      "acumio",
                      model::Description_SourceCategory_ACUMIO_INSTALLATION,
                      "installation",
                      &root_description);
  EXPECT_OK(repository.AddWithDescription(root, root_description));
  EXPECT_OK(repository.AddWithNoDescription(foo));
  ASSERT_EQ(repository.size(), 2);
  EXPECT_FALSE(foo.is_repository_name());
  foo.set_is_repository_name(true);
  repository.UpdateNoDescription("foo", foo);
  model::Namespace found_foo;
  EXPECT_OK(repository.GetNamespace("foo", &found_foo));
  EXPECT_TRUE(found_foo.is_repository_name());
}

TEST(NamespaceRepository, UpdateWithKeyChange) {
  model::Namespace root;
  model::Namespace foo;
  model::Namespace bar;
  model::Description root_description;
  NamespaceRepository repository;
  PopulateRootNamespace(&root);
  SetToNamespaceFoo(root, &foo);
  SetToNamespaceBar(root, &bar);
  PopulateDescription("root Namespace: required at installation",
                      "acumio",
                      model::Description_SourceCategory_ACUMIO_INSTALLATION,
                      "installation",
                      &root_description);
  EXPECT_OK(repository.AddWithDescription(root, root_description));
  EXPECT_OK(repository.AddWithNoDescription(foo));
  EXPECT_OK(repository.AddWithNoDescription(bar));
  ASSERT_EQ(repository.size(), 3);
  SetToNamespaceCat(root, &bar);
  EXPECT_OK(repository.UpdateNoDescription("bar", bar));
  ASSERT_EQ(repository.size(), 3);
  SetToNamespaceBar(root, &bar);
  EXPECT_OK(repository.UpdateNoDescription("cat", bar));
  ASSERT_EQ(repository.size(), 3);
  SetToNamespaceZebra(root, &bar);
  EXPECT_OK(repository.UpdateNoDescription("bar", bar));
  ASSERT_EQ(repository.size(), 3);
  SetToNamespaceBar(root, &bar);
  EXPECT_OK(repository.UpdateNoDescription("zebra", bar));
  ASSERT_EQ(repository.size(), 3);
  SetToNamespaceCat(root, &foo);
  EXPECT_OK(repository.UpdateNoDescription("foo", foo));
  ASSERT_EQ(repository.size(), 3);
  SetToNamespaceFoo(root, &foo);
  EXPECT_OK(repository.UpdateNoDescription("cat", foo));
  ASSERT_EQ(repository.size(), 3);
  SetToNamespaceFoo(root, &bar);
  EXPECT_ERROR(repository.UpdateNoDescription("bar", bar),
               grpc::StatusCode::ALREADY_EXISTS);
  ASSERT_EQ(repository.size(), 3);
  SetToNamespaceBar(root, &foo);
  EXPECT_ERROR(repository.UpdateNoDescription("foo", foo),
               grpc::StatusCode::ALREADY_EXISTS);
  ASSERT_EQ(repository.size(), 3);
}

TEST(NamespaceRepository, DescriptionUpdates) {
  model::Namespace root;
  model::Namespace foo;
  model::Description root_description;
  NamespaceRepository repository;
  PopulateRootNamespace(&root);
  SetToNamespaceFoo(root, &foo);
  std::string contents = "root Namespace: required at installation";
  PopulateDescription(contents,
                      "acumio",
                      model::Description_SourceCategory_ACUMIO_INSTALLATION,
                      "installation",
                      &root_description);
  EXPECT_OK(repository.AddWithDescription(root, root_description));
  EXPECT_OK(repository.AddWithNoDescription(foo));
  ASSERT_EQ(repository.size(), 2);
  model::Description found_description;
  EXPECT_OK(repository.GetDescription("", &found_description));
  EXPECT_EQ(found_description.contents(), contents);
  model::DescriptionHistory found_history;
  EXPECT_OK(repository.GetDescriptionHistory("", &found_history));
  ASSERT_EQ(found_history.version_size(), 1);
  ASSERT_EQ(found_history.version(0).contents(), contents);
  ASSERT_EQ(found_history.version(0).editor(), "acumio");
  ASSERT_EQ(found_history.version(0).knowledge_source_category(),
            model::Description_SourceCategory_ACUMIO_INSTALLATION);
  ASSERT_EQ(found_history.version(0).knowledge_source(), "installation");
  EXPECT_OK(repository.GetDescription("foo", &found_description));
  EXPECT_FALSE(found_description.has_edit_time());
  EXPECT_EQ(found_description.contents(), "");
  EXPECT_OK(repository.GetDescriptionHistory("foo", &found_history));
  EXPECT_EQ(found_history.version_size(), 0);
  root_description.set_contents("fish");
  EXPECT_OK(repository.UpdateDescriptionOnly("", root_description));
  EXPECT_OK(repository.GetDescription("", &found_description));
  EXPECT_EQ(found_description.contents(), "fish");
  EXPECT_OK(repository.GetDescriptionHistory("", &found_history));
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
  EXPECT_OK(repository.ClearDescription(""));
  EXPECT_OK(repository.GetDescriptionHistory("", &found_history));
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
  EXPECT_OK(repository.ClearDescription(""));
  EXPECT_OK(repository.GetDescriptionHistory("", &found_history));
  // And here is how we verify it: there should be no new versions of
  // the DescriptionHistory after this clearing the description twice
  // in a row.
  ASSERT_EQ(found_history.version_size(), 3);

  root.set_is_repository_name(true);
  root_description.set_contents(contents);
  EXPECT_OK(repository.UpdateWithDescription("", root, root_description));
  EXPECT_OK(repository.GetDescriptionHistory("", &found_history));
  ASSERT_EQ(found_history.version_size(), 4);
  EXPECT_EQ(found_history.version(3).contents(), contents);
  model::Namespace found_root;
  EXPECT_OK(repository.GetNamespaceAndDescriptionHistory(
      "", &found_root, &found_history));
  EXPECT_TRUE(found_root.is_repository_name());
  ASSERT_EQ(found_history.version_size(), 4);
  EXPECT_EQ(found_history.version(3).contents(), contents);

  root.set_is_repository_name(false);
  EXPECT_OK(repository.UpdateAndClearDescription("", root));
  EXPECT_OK(repository.GetNamespaceAndDescriptionHistory(
      "", &found_root, &found_history));
  EXPECT_FALSE(found_root.is_repository_name());
  ASSERT_EQ(found_history.version_size(), 5);
  EXPECT_EQ(found_history.version(4).contents(), "");
}

// TODO: Tests for Primary and Secondary Iterators and tests for
// secondary index changes.
TEST(NamespaceRepository, PrimaryIndex) {
  model::Namespace root;
  model::Namespace foo;
  model::Namespace bar;
  model::Namespace cat;
  model::Namespace dog;
  model::Namespace internet;
  model::Namespace bills_house;
  model::Description root_description;
  model::Description cat_description;
  model::Description dog_description;
  model::Description internet_description;
  model::Description bills_house_description;
  PopulateRootNamespace(&root);
  SetToNamespaceFoo(root, &foo);
  SetToNamespaceBar(root, &bar);
  SetToNamespaceCat(root, &cat);
  SetToNamespaceDog(root, &dog);
  PopulateNamespace(cat, "internet", "/", true, &internet);
  PopulateNamespace(cat, "bills_house", "/", true, &bills_house);
  PopulateDescription("root Namespace: required at installation",
                      "acumio",
                      model::Description_SourceCategory_ACUMIO_INSTALLATION,
                      "installation",
                      &root_description);
  PopulateDescription("Namespace for things related to cats",
                      "bill",
                      model::Description_SourceCategory_HUMAN_DOMAIN_KNOWLEDGE,
                      "bill's head",
                      &cat_description);
  PopulateDescription("Namespace for things related to dogs",
                      "bill",
                      model::Description_SourceCategory_HUMAN_DOMAIN_KNOWLEDGE,
                      "bill's head",
                      &dog_description);
  PopulateDescription("The Amazing and Ultimate Cat Video Database",
                      "bill",
                      model::Description_SourceCategory_HUMAN_DOMAIN_KNOWLEDGE,
                      "bill's head",
                      &internet_description);
  PopulateDescription("Repository for the cat 'Boo-Boo'",
                      "bill",
                      model::Description_SourceCategory_HUMAN_DOMAIN_KNOWLEDGE,
                      "bill's head",
                      &bills_house_description);

  NamespaceRepository repository;
  EXPECT_OK(repository.AddWithDescription(root, root_description));
  EXPECT_OK(repository.AddWithNoDescription(foo));
  EXPECT_OK(repository.AddWithNoDescription(bar));
  EXPECT_OK(repository.AddWithDescription(cat, cat_description));
  EXPECT_OK(repository.AddWithDescription(dog, dog_description));
  EXPECT_OK(repository.AddWithDescription(internet, internet_description));
  EXPECT_OK(repository.AddWithDescription(bills_house,
                                          bills_house_description));
  ASSERT_EQ(repository.size(), 7);
  std::vector<std::string> primary_order;
  primary_order.push_back("");
  primary_order.push_back("bar");
  primary_order.push_back("cat");
  primary_order.push_back("cat::bills_house");
  primary_order.push_back("cat::internet");
  primary_order.push_back("dog");
  primary_order.push_back("foo");
  ASSERT_EQ(primary_order.size(), 7);
  EXPECT_OK(CompareStringVectorToPrimaryIterator(primary_order,
                                                 repository.primary_begin(),
                                                 repository.primary_end()));
  primary_order.erase(primary_order.begin(), primary_order.begin()+2);
  ASSERT_EQ(primary_order.size(), 5);
  EXPECT_OK(CompareStringVectorToPrimaryIterator(
      primary_order,
      repository.LowerBoundByFullName("c"),
      repository.primary_end()));
  EXPECT_OK(CompareStringVectorToPrimaryIterator(
      primary_order,
      repository.LowerBoundByFullName("cat"),
      repository.primary_end()));

  std::vector<std::string> secondary_order;
  secondary_order.push_back("");
  secondary_order.push_back("bar");
  secondary_order.push_back("bills_house");
  secondary_order.push_back("cat");
  secondary_order.push_back("dog");
  secondary_order.push_back("foo");
  secondary_order.push_back("internet");
  ASSERT_EQ(secondary_order.size(), 7);
  EXPECT_OK(CompareStringVectorToSecondaryIterator(
      secondary_order,
      repository.short_name_begin(),
      repository.short_name_end()));
  secondary_order.erase(secondary_order.begin());
  ASSERT_EQ(secondary_order.size(), 6);
  EXPECT_OK(CompareStringVectorToSecondaryIterator(
      secondary_order,
      repository.LowerBoundByShortName("a"),
      repository.short_name_end()));
}

} // anonymous namespace
} // namespace acumio

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
