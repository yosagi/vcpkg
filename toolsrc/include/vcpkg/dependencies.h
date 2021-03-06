#pragma once

#include <vcpkg/base/optional.h>
#include <vcpkg/base/util.h>
#include <vcpkg/build.h>
#include <vcpkg/packagespec.h>
#include <vcpkg/statusparagraphs.h>
#include <vcpkg/vcpkgpaths.h>

#include <vector>

namespace vcpkg::Dependencies
{
    enum class RequestType
    {
        UNKNOWN,
        USER_REQUESTED,
        AUTO_SELECTED
    };

    std::string to_output_string(RequestType request_type,
                                 const CStringView s,
                                 const Build::BuildPackageOptions& options);
    std::string to_output_string(RequestType request_type, const CStringView s);

    enum class InstallPlanType
    {
        UNKNOWN,
        BUILD_AND_INSTALL,
        ALREADY_INSTALLED,
        EXCLUDED
    };

    struct InstallPlanAction : Util::MoveOnlyBase
    {
        static bool compare_by_name(const InstallPlanAction* left, const InstallPlanAction* right);

        InstallPlanAction();

        InstallPlanAction(const PackageSpec& spec,
                          InstalledPackageView&& spghs,
                          const std::unordered_set<std::string>& features,
                          const RequestType& request_type);

        InstallPlanAction(const PackageSpec& spec,
                          const SourceControlFile& scf,
                          const std::unordered_set<std::string>& features,
                          const RequestType& request_type);

        std::string displayname() const;

        PackageSpec spec;

        Optional<const SourceControlFile&> source_control_file;
        Optional<InstalledPackageView> installed_package;

        InstallPlanType plan_type;
        RequestType request_type;
        Build::BuildPackageOptions build_options;
        std::unordered_set<std::string> feature_list;
    };

    enum class RemovePlanType
    {
        UNKNOWN,
        NOT_INSTALLED,
        REMOVE
    };

    struct RemovePlanAction : Util::MoveOnlyBase
    {
        static bool compare_by_name(const RemovePlanAction* left, const RemovePlanAction* right);

        RemovePlanAction();
        RemovePlanAction(const PackageSpec& spec, const RemovePlanType& plan_type, const RequestType& request_type);

        PackageSpec spec;
        RemovePlanType plan_type;
        RequestType request_type;
    };

    struct AnyAction
    {
        AnyAction(InstallPlanAction&& iplan) : install_action(std::move(iplan)) {}
        AnyAction(RemovePlanAction&& rplan) : remove_action(std::move(rplan)) {}

        Optional<InstallPlanAction> install_action;
        Optional<RemovePlanAction> remove_action;

        const PackageSpec& spec() const;
    };

    enum class ExportPlanType
    {
        UNKNOWN,
        PORT_AVAILABLE_BUT_NOT_BUILT,
        ALREADY_BUILT
    };

    struct ExportPlanAction : Util::MoveOnlyBase
    {
        static bool compare_by_name(const ExportPlanAction* left, const ExportPlanAction* right);

        ExportPlanAction();
        ExportPlanAction(const PackageSpec& spec,
                         InstalledPackageView&& installed_package,
                         const RequestType& request_type);

        ExportPlanAction(const PackageSpec& spec, const RequestType& request_type);

        PackageSpec spec;
        ExportPlanType plan_type;
        RequestType request_type;

        Optional<const BinaryParagraph&> core_paragraph() const;
        std::vector<PackageSpec> dependencies(const Triplet& triplet) const;

    private:
        Optional<InstalledPackageView> m_installed_package;
    };

    struct PortFileProvider
    {
        virtual Optional<const SourceControlFile&> get_control_file(const std::string& src_name) const = 0;
    };

    struct MapPortFileProvider : Util::ResourceBase, PortFileProvider
    {
        explicit MapPortFileProvider(const std::unordered_map<std::string, SourceControlFile>& map);
        Optional<const SourceControlFile&> get_control_file(const std::string& src_name) const override;

    private:
        const std::unordered_map<std::string, SourceControlFile>& ports;
    };

    struct PathsPortFileProvider : Util::ResourceBase, PortFileProvider
    {
        explicit PathsPortFileProvider(const VcpkgPaths& paths);
        Optional<const SourceControlFile&> get_control_file(const std::string& src_name) const override;

    private:
        const VcpkgPaths& ports;
        mutable std::unordered_map<std::string, SourceControlFile> cache;
    };

    struct ClusterGraph;
    struct GraphPlan;

    struct PackageGraph
    {
        PackageGraph(const PortFileProvider& provider, const StatusParagraphs& status_db);
        ~PackageGraph();

        void install(const FeatureSpec& spec,
                     const std::unordered_set<std::string>& prevent_default_features = {}) const;
        void upgrade(const PackageSpec& spec) const;

        std::vector<AnyAction> serialize() const;

    private:
        std::unique_ptr<GraphPlan> m_graph_plan;
        std::unique_ptr<ClusterGraph> m_graph;
    };

    std::vector<InstallPlanAction> create_install_plan(const PortFileProvider& port_file_provider,
                                                       const std::vector<PackageSpec>& specs,
                                                       const StatusParagraphs& status_db);

    std::vector<RemovePlanAction> create_remove_plan(const std::vector<PackageSpec>& specs,
                                                     const StatusParagraphs& status_db);

    std::vector<ExportPlanAction> create_export_plan(const PortFileProvider& port_file_provider,
                                                     const VcpkgPaths& paths,
                                                     const std::vector<PackageSpec>& specs,
                                                     const StatusParagraphs& status_db);

    std::vector<AnyAction> create_feature_install_plan(const std::unordered_map<std::string, SourceControlFile>& map,
                                                       const std::vector<FeatureSpec>& specs,
                                                       const StatusParagraphs& status_db);

    std::vector<AnyAction> create_feature_install_plan(const PortFileProvider& port_file_provider,
                                                       const std::vector<FeatureSpec>& specs,
                                                       const StatusParagraphs& status_db);

    void print_plan(const std::vector<AnyAction>& action_plan, const bool is_recursive = true);
}
