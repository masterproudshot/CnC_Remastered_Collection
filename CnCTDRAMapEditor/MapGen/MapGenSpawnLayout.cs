//
// Spawn layout names, reference survey cells, and recipe default resolution.
//
using System;
using System.Collections.Generic;
using System.Drawing;
using System.Linq;
using MobiusEditor.Model;

namespace MobiusEditor.MapGen
{
    public static class MapGenTerrainProfiles
    {
        public const string Flat = "flat";
        public const string TemperateMixed = "temperate-mixed";

        private static readonly string[] Known = { Flat, TemperateMixed };

        public static bool TryNormalize(string value, out string normalized, out string error)
        {
            normalized = null;
            error = null;
            if (string.IsNullOrWhiteSpace(value))
            {
                normalized = Flat;
                return true;
            }

            normalized = value.Trim().ToLowerInvariant();
            if (!Known.Contains(normalized, StringComparer.Ordinal))
            {
                error = "Unknown TerrainProfile '" + value + "'. Known: " + string.Join(", ", Known) + ".";
                return false;
            }

            return true;
        }
    }

    public static class MapGenSpawnLayouts
    {
        public const string Corners8 = "corners8";
        public const string OctagonOpen = "octagonOpen";
        public const string MiddleRoad = "middleRoad";

        private static readonly string[] Known = { Corners8, OctagonOpen, MiddleRoad };

        /// <summary>Reference cells for 126×126 maps at MapTile (1,1). Octagon (Open) V1.4 survey.</summary>
        public static readonly int[] ReferenceOctagonOpen126 =
        {
            1967, 6031, 10255, 14383, 14416, 10352, 6128, 2000
        };

        /// <summary>Reference cells for 126×126 maps at MapTile (1,1). Middle Road 2-6p survey.</summary>
        public static readonly int[] ReferenceMiddleRoad126 =
        {
            5289, 5334, 12174, 12192, 12224, 5311, 12255, 12273
        };

        /// <summary>
        /// No-shortage / theta lineage (catalog cross-reference; same family as octagonOpen for 8p 126×126).
        /// </summary>
        public static readonly int[] ReferenceNoShortage126 =
        {
            2321, 2367, 2413, 8083, 8172, 13842, 13887, 13933
        };

        /// <summary>CLI / recipe aliases accepted by TryNormalize (B4).</summary>
        private static readonly Dictionary<string, string> Aliases = new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase)
        {
            { "octagon8", OctagonOpen },
            { "middle-road", MiddleRoad },
        };

        public static bool TryNormalize(string value, out string normalized, out string error)
        {
            normalized = null;
            error = null;
            if (string.IsNullOrWhiteSpace(value))
            {
                normalized = Corners8;
                return true;
            }

            var trimmed = value.Trim();
            if (Aliases.TryGetValue(trimmed, out string alias))
            {
                normalized = alias;
                return true;
            }

            normalized = Known.FirstOrDefault(k => string.Equals(k, trimmed, StringComparison.OrdinalIgnoreCase));
            if (normalized == null)
            {
                error = "Unknown SpawnLayout '" + value + "'. Known: " + string.Join(", ", Known) +
                        "; aliases: octagon8, middle-road.";
                return false;
            }

            return true;
        }
    }

    public sealed class SpawnLayoutResolution
    {
        public string Layout { get; internal set; }
        public int WaypointCount { get; internal set; }
        public bool UsesReferenceCells { get; internal set; }
        public IReadOnlyList<int> ReferenceCells { get; internal set; }
        public string PlacementNote { get; internal set; }
    }

    public static class MapGenRecipeValidation
    {
        public const int DefaultMapSize = 64;
        public const int DefaultMapSize8Players = 126;

        public static void ApplyDefaults(MapGenRecipe recipe)
        {
            if (recipe == null)
            {
                return;
            }

            if (recipe.Players == 8 && recipe.MapSize == DefaultMapSize)
            {
                recipe.MapSize = DefaultMapSize8Players;
            }
        }

        public static IList<string> ValidateAndResolve(MapGenRecipe recipe, out SpawnLayoutResolution resolution)
        {
            resolution = null;
            var errors = new List<string>();
            if (recipe == null)
            {
                errors.Add("Recipe is null.");
                return errors;
            }

            ApplyDefaults(recipe);

            if (recipe.Players < 2 || recipe.Players > 8)
            {
                errors.Add("Players must be between 2 and 8.");
            }
            if (recipe.MapSize < 16 || recipe.MapSize > 126)
            {
                errors.Add("MapSize must be between 16 and 126.");
            }

            if (!MapGenTerrainProfiles.TryNormalize(recipe.TerrainProfile, out string terrainProfile, out string terrainError))
            {
                errors.Add(terrainError);
            }
            else
            {
                recipe.TerrainProfile = terrainProfile;
            }

            if (!MapGenSpawnLayouts.TryNormalize(recipe.SpawnLayout, out string spawnLayout, out string layoutError))
            {
                errors.Add(layoutError);
            }
            else
            {
                recipe.SpawnLayout = spawnLayout;
            }

            if (errors.Count > 0)
            {
                return errors;
            }

            resolution = ResolveSpawnLayout(recipe);
            if (resolution.WaypointCount < recipe.Players)
            {
                errors.Add("SpawnLayout '" + recipe.SpawnLayout + "' resolves to " + resolution.WaypointCount +
                           " waypoints but recipe requests " + recipe.Players + " players.");
            }

            if ((recipe.SpawnLayout == MapGenSpawnLayouts.OctagonOpen ||
                 recipe.SpawnLayout == MapGenSpawnLayouts.MiddleRoad) &&
                recipe.MapSize != 126)
            {
                errors.Add("SpawnLayout '" + recipe.SpawnLayout + "' requires MapSize 126 (reference survey is 126×126).");
            }

            return errors;
        }

        public static SpawnLayoutResolution ResolveSpawnLayout(MapGenRecipe recipe)
        {
            int waypointCount = Math.Min(recipe.Players, 8);
            switch (recipe.SpawnLayout)
            {
                case MapGenSpawnLayouts.OctagonOpen:
                    return new SpawnLayoutResolution
                    {
                        Layout = MapGenSpawnLayouts.OctagonOpen,
                        WaypointCount = waypointCount,
                        UsesReferenceCells = true,
                        ReferenceCells = MapGenSpawnLayouts.ReferenceOctagonOpen126,
                        PlacementNote = "Octagon (Open) V1.4 catalog cells (CUSTOM-MAPS-CATALOG.md)."
                    };
                case MapGenSpawnLayouts.MiddleRoad:
                    return new SpawnLayoutResolution
                    {
                        Layout = MapGenSpawnLayouts.MiddleRoad,
                        WaypointCount = waypointCount,
                        UsesReferenceCells = true,
                        ReferenceCells = MapGenSpawnLayouts.ReferenceMiddleRoad126,
                        PlacementNote = "Middle Road 2-6p catalog cells (CUSTOM-MAPS-CATALOG.md)."
                    };
                default:
                    return new SpawnLayoutResolution
                    {
                        Layout = MapGenSpawnLayouts.Corners8,
                        WaypointCount = waypointCount,
                        UsesReferenceCells = false,
                        ReferenceCells = Array.Empty<int>(),
                        PlacementNote = "corners8: four corners + four edge midpoints on playable rect."
                    };
            }
        }
    }

    public static class MapGenSpawnTiles
    {
        /// <summary>Resolve spawn tile locations without assigning waypoints (for terrain pad clearing).</summary>
        public static List<Point> Resolve(Map map, int players, SpawnLayoutResolution layout)
        {
            if (layout == null)
            {
                return new List<Point>();
            }

            if (layout.UsesReferenceCells)
            {
                return ResolveReferenceTiles(map, players, layout.ReferenceCells);
            }

            return ResolveCorners8Tiles(map, players);
        }

        private static List<Point> ResolveCorners8Tiles(Map map, int players)
        {
            var bounds = map.Bounds;
            int inset = 6;
            var corners = new[]
            {
                new Point(bounds.Left + inset, bounds.Top + inset),
                new Point(bounds.Right - inset - 1, bounds.Top + inset),
                new Point(bounds.Left + inset, bounds.Bottom - inset - 1),
                new Point(bounds.Right - inset - 1, bounds.Bottom - inset - 1),
                new Point(bounds.Left + bounds.Width / 2, bounds.Top + inset),
                new Point(bounds.Left + bounds.Width / 2, bounds.Bottom - inset - 1),
                new Point(bounds.Left + inset, bounds.Top + bounds.Height / 2),
                new Point(bounds.Right - inset - 1, bounds.Top + bounds.Height / 2),
            };

            return CollectValidTiles(map, players, corners);
        }

        private static List<Point> ResolveReferenceTiles(Map map, int players, IReadOnlyList<int> referenceCells)
        {
            const int globalWidth = 128;
            int count = Math.Min(players, referenceCells.Count);
            var tiles = new Point[count];
            for (int i = 0; i < count; i++)
            {
                int globalCell = referenceCells[i];
                tiles[i] = new Point(globalCell % globalWidth, globalCell / globalWidth);
            }

            return CollectValidTiles(map, players, tiles);
        }

        private static List<Point> CollectValidTiles(Map map, int players, IReadOnlyList<Point> tiles)
        {
            var spawns = new List<Point>();
            for (int i = 0; i < players && i < tiles.Count; i++)
            {
                var tile = tiles[i];
                if (!map.Metrics.GetCell(tile, out _))
                {
                    continue;
                }
                spawns.Add(tile);
            }

            return spawns;
        }
    }
}