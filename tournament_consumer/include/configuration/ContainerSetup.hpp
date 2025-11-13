// tournament_consumer/include/configuration/ContainerSetup.hpp
#pragma once
#include <memory>
#include <Hypodermic/Hypodermic.h>

#include "persistence/repository/MatchRepository.hpp"
#include "persistence/repository/TeamRepository.hpp"
#include "processors/TeamAddedEventProcessor.hpp"
#include "processors/ScoreRecordedEventProcessor.hpp"

namespace config {

    inline std::shared_ptr<Hypodermic::Container> containerSetup() {
        Hypodermic::ContainerBuilder builder;

        // Registrar como tipos concretos (self). NO uses .as<T>() con el mismo tipo.
        builder.registerType<persistence::MatchRepository>()
               .singleInstance();

        builder.registerType<persistence::TeamRepository>()
               .singleInstance();

        // Processors (Hypodermic inyecta por ctor)
        builder.registerType<consumers::TeamAddedEventProcessor>()
               .singleInstance();

        builder.registerType<consumers::ScoreRecordedEventProcessor>()
               .singleInstance();

        return builder.build();
    }

} // namespace config
