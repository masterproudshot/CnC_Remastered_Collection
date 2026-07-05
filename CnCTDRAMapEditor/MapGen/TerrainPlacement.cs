//
// Terrain placement — flat v1 (clear fill) and temperate-mixed v2 (water/rock/cliff bands, trees, rough).
//
using MobiusEditor.Model;
using MobiusEditor.RedAlert;
using System;
using System.Collections.Generic;
using System.Drawing;
using System.Linq;

namespace MobiusEditor.MapGen
{
    public static class TerrainPlacement
    {
        private static readonly string[] TreeTypeNames =
        {
            "t01", "t02", "t03", "t05", "t06", "t07", "t08", "t10", "t11", "t12", "t13", "t14", "t15", "t16", "t17"
        };

        private static readonly string[] RoughTypeNames =
        {
            "rf01", "rf02", "rf03", "rf04", "rf05", "rf06", "rf07"
        };

        private static readonly string[] RockTypeNames = { "b1", "b2", "b3" };

        private static readonly string[] CliffTypeNames =
        {
            "wc01", "wc02", "wc03", "wc04", "wc05", "wc06", "wc07", "wc08", "wc09", "wc10",
            "wc11", "wc12", "wc13", "wc14", "wc15"
        };

        public sealed class Options
        {
            public string TerrainProfile { get; set; } = MapGenTerrainProfiles.Flat;
            public int SpawnPadRadius { get; set; } = 5;
        }

        public static void Apply(
            Map map,
            Random random,
            IReadOnlyList<Point> spawnTiles,
            Options options)
        {
            if (options == null)
            {
                throw new ArgumentNullException(nameof(options));
            }

            if (string.Equals(options.TerrainProfile, MapGenTerrainProfiles.Flat, StringComparison.Ordinal))
            {
                FillClearTerrain(map);
                return;
            }

            if (string.Equals(options.TerrainProfile, MapGenTerrainProfiles.TemperateMixed, StringComparison.Ordinal))
            {
                ApplyTemperateMixed(map, random, spawnTiles, options);
                return;
            }
        }

        private static void ApplyTemperateMixed(
            Map map,
            Random random,
            IReadOnlyList<Point> spawnTiles,
            Options options)
        {
            FillClearTerrain(map);

            var bounds = map.Bounds;
            int padRadius = options.SpawnPadRadius;
            var waterCells = new HashSet<Point>();
            var cliffCells = new HashSet<Point>();

            PlaceWaterBands(map, random, bounds, spawnTiles, padRadius, waterCells);
            PlaceCliffBands(map, random, bounds, spawnTiles, padRadius, waterCells, cliffCells);
            PlaceRockBands(map, random, bounds, spawnTiles, padRadius, cliffCells, waterCells);
            PlaceRoughPatches(map, random, bounds, spawnTiles, padRadius + 1, waterCells, cliffCells);
            PlaceTreePatches(map, random, bounds, spawnTiles, padRadius + 1, waterCells, cliffCells);
            ClearSpawnPads(map, spawnTiles, padRadius);
        }

        private static void FillClearTerrain(Map map)
        {
            var clearType = map.TemplateTypes.FirstOrDefault(t => t.Equals("clear1"));
            if (clearType == null)
            {
                return;
            }

            foreach (var p in EnumerateRect(map.Bounds))
            {
                map.Templates[p] = new Template { Type = clearType, Icon = 0 };
            }
        }

        private static void ClearSpawnPads(Map map, IReadOnlyList<Point> spawnTiles, int radius)
        {
            var clearType = map.TemplateTypes.FirstOrDefault(t => t.Equals("clear1"));
            if (clearType == null)
            {
                return;
            }

            foreach (var spawn in spawnTiles)
            {
                foreach (var p in EnumerateRect(map.Bounds))
                {
                    if (!IsNear(spawn, p, radius))
                    {
                        continue;
                    }
                    if (!map.Metrics.GetCell(p, out _))
                    {
                        continue;
                    }
                    map.Templates[p] = new Template { Type = clearType, Icon = 0 };
                }
            }
        }

        private static void PlaceWaterBands(
            Map map,
            Random random,
            Rectangle bounds,
            IReadOnlyList<Point> spawnTiles,
            int padRadius,
            HashSet<Point> waterCells)
        {
            var waterType = map.TemplateTypes.FirstOrDefault(t => t.Equals("w1"));
            if (waterType == null)
            {
                return;
            }

            int bandWidth = Math.Max(3, bounds.Width / 14);
            bool horizontal = random.Next(2) == 0;
            int center = horizontal
                ? bounds.Top + bounds.Height / 2 + random.Next(-bounds.Height / 8, bounds.Height / 8 + 1)
                : bounds.Left + bounds.Width / 2 + random.Next(-bounds.Width / 8, bounds.Width / 8 + 1);

            foreach (var p in EnumerateRect(bounds))
            {
                if (IsNearAnySpawn(p, spawnTiles, padRadius + 2))
                {
                    continue;
                }

                bool inBand;
                if (horizontal)
                {
                    inBand = Math.Abs(p.Y - center) < bandWidth;
                }
                else
                {
                    inBand = Math.Abs(p.X - center) < bandWidth;
                }

                if (!inBand)
                {
                    continue;
                }

                if (!TrySetTemplate(map, p, waterType, 0))
                {
                    continue;
                }
                waterCells.Add(p);
            }

            // Secondary edge pond on one map border.
            int edgeBand = Math.Max(2, bounds.Width / 20);
            int edge = random.Next(4);
            foreach (var p in EnumerateRect(bounds))
            {
                if (IsNearAnySpawn(p, spawnTiles, padRadius + 2))
                {
                    continue;
                }

                bool inEdgeBand = false;
                switch (edge)
                {
                    case 0: inEdgeBand = p.Y < bounds.Top + edgeBand; break;
                    case 1: inEdgeBand = p.Y >= bounds.Bottom - edgeBand; break;
                    case 2: inEdgeBand = p.X < bounds.Left + edgeBand; break;
                    case 3: inEdgeBand = p.X >= bounds.Right - edgeBand; break;
                }

                if (!inEdgeBand || random.NextDouble() > 0.72)
                {
                    continue;
                }

                if (!TrySetTemplate(map, p, waterType, 0))
                {
                    continue;
                }
                waterCells.Add(p);
            }
        }

        private static void PlaceCliffBands(
            Map map,
            Random random,
            Rectangle bounds,
            IReadOnlyList<Point> spawnTiles,
            int padRadius,
            HashSet<Point> waterCells,
            HashSet<Point> cliffCells)
        {
            if (waterCells.Count == 0)
            {
                return;
            }

            int cliffRing = 2;
            foreach (var water in waterCells)
            {
                foreach (var n in Neighbors8(water))
                {
                    if (!bounds.Contains(n) || waterCells.Contains(n))
                    {
                        continue;
                    }
                    if (IsNearAnySpawn(n, spawnTiles, padRadius + 1))
                    {
                        continue;
                    }
                    cliffCells.Add(n);
                }

                foreach (var n in Neighbors8(water))
                {
                    foreach (var n2 in Neighbors8(n))
                    {
                        if (!bounds.Contains(n2) || waterCells.Contains(n2) || cliffCells.Contains(n2))
                        {
                            continue;
                        }
                        if (IsNearAnySpawn(n2, spawnTiles, padRadius + 1))
                        {
                            continue;
                        }
                        if (MinDistanceToSet(n2, waterCells) <= cliffRing + 1)
                        {
                            cliffCells.Add(n2);
                        }
                    }
                }
            }

            foreach (var p in cliffCells)
            {
                if (random.NextDouble() > 0.82)
                {
                    continue;
                }
                var cliffName = CliffTypeNames[random.Next(CliffTypeNames.Length)];
                var cliffType = map.TemplateTypes.FirstOrDefault(t => t.Equals(cliffName));
                if (cliffType == null)
                {
                    continue;
                }
                TrySetTemplate(map, p, cliffType, random.Next(Math.Max(1, cliffType.NumIcons)));
            }
        }

        private static void PlaceRockBands(
            Map map,
            Random random,
            Rectangle bounds,
            IReadOnlyList<Point> spawnTiles,
            int padRadius,
            HashSet<Point> cliffCells,
            HashSet<Point> waterCells)
        {
            int target = Math.Max(12, (bounds.Width * bounds.Height) / 180);
            int placed = 0;
            int guard = 0;

            while (placed < target && guard++ < target * 20)
            {
                var p = new Point(
                    random.Next(bounds.Left, bounds.Right),
                    random.Next(bounds.Top, bounds.Bottom));
                if (!bounds.Contains(p))
                {
                    continue;
                }
                if (IsNearAnySpawn(p, spawnTiles, padRadius + 1))
                {
                    continue;
                }
                if (waterCells.Contains(p))
                {
                    continue;
                }

                bool nearCliff = cliffCells.Any(c => IsNear(c, p, 3));
                if (!nearCliff && random.NextDouble() > 0.35)
                {
                    continue;
                }

                var rockName = RockTypeNames[random.Next(RockTypeNames.Length)];
                var rockType = map.TemplateTypes.FirstOrDefault(t => t.Equals(rockName));
                if (rockType == null)
                {
                    continue;
                }
                if (!TrySetTemplate(map, p, rockType, random.Next(Math.Max(1, rockType.NumIcons))))
                {
                    continue;
                }
                placed++;
            }
        }

        private static void PlaceRoughPatches(
            Map map,
            Random random,
            Rectangle bounds,
            IReadOnlyList<Point> spawnTiles,
            int avoidRadius,
            HashSet<Point> waterCells,
            HashSet<Point> cliffCells)
        {
            var roughTypes = RoughTypeNames
                .Select(n => map.TemplateTypes.FirstOrDefault(t => t.Equals(n)))
                .Where(t => t != null)
                .ToList();
            if (roughTypes.Count == 0)
            {
                return;
            }

            int patchCount = Math.Max(4, bounds.Width / 12);
            int cellsPerPatch = Math.Max(18, bounds.Width / 3);

            for (int s = 0; s < patchCount; s++)
            {
                var seed = new Point(
                    random.Next(bounds.Left, bounds.Right),
                    random.Next(bounds.Top, bounds.Bottom));
                if (IsBlockedForScatter(seed, spawnTiles, avoidRadius, waterCells, cliffCells))
                {
                    continue;
                }

                GrowTemplatePatch(
                    map, random, seed, cellsPerPatch, spawnTiles, avoidRadius, waterCells, cliffCells,
                    spreadChance: 0.74,
                    pickType: r => roughTypes[r.Next(roughTypes.Count)]);
            }
        }

        private static void PlaceTreePatches(
            Map map,
            Random random,
            Rectangle bounds,
            IReadOnlyList<Point> spawnTiles,
            int avoidRadius,
            HashSet<Point> waterCells,
            HashSet<Point> cliffCells)
        {
            var treeTypes = TreeTypeNames
                .Select(n => map.TerrainTypes.FirstOrDefault(t => t.Equals(n)))
                .Where(t => t != null)
                .ToList();
            if (treeTypes.Count == 0)
            {
                return;
            }

            int patchCount = Math.Max(5, bounds.Width / 10);
            int treesPerPatch = Math.Max(10, bounds.Width / 5);

            for (int s = 0; s < patchCount; s++)
            {
                var seed = new Point(
                    random.Next(bounds.Left, bounds.Right),
                    random.Next(bounds.Top, bounds.Bottom));
                if (IsBlockedForScatter(seed, spawnTiles, avoidRadius, waterCells, cliffCells))
                {
                    continue;
                }

                GrowTreePatch(map, random, seed, treesPerPatch, spawnTiles, avoidRadius, waterCells, cliffCells, treeTypes);
            }
        }

        private static void GrowTemplatePatch(
            Map map,
            Random random,
            Point seed,
            int patchGoal,
            IReadOnlyList<Point> spawnTiles,
            int avoidRadius,
            HashSet<Point> waterCells,
            HashSet<Point> cliffCells,
            double spreadChance,
            Func<Random, TemplateType> pickType)
        {
            var queue = new Queue<Point>();
            queue.Enqueue(seed);
            int placed = 0;
            int guard = 0;

            while (queue.Count > 0 && placed < patchGoal && guard++ < 8000)
            {
                var p = queue.Dequeue();
                if (IsBlockedForScatter(p, spawnTiles, avoidRadius, waterCells, cliffCells))
                {
                    continue;
                }
                if (!map.Metrics.GetCell(p, out _))
                {
                    continue;
                }

                var type = pickType(random);
                if (!TrySetTemplate(map, p, type, random.Next(Math.Max(1, type.NumIcons))))
                {
                    continue;
                }
                placed++;

                foreach (var n in Neighbors8(p))
                {
                    if (!map.Bounds.Contains(n))
                    {
                        continue;
                    }
                    if (random.NextDouble() < spreadChance)
                    {
                        queue.Enqueue(n);
                    }
                }
            }
        }

        private static void GrowTreePatch(
            Map map,
            Random random,
            Point seed,
            int patchGoal,
            IReadOnlyList<Point> spawnTiles,
            int avoidRadius,
            HashSet<Point> waterCells,
            HashSet<Point> cliffCells,
            List<TerrainType> treeTypes)
        {
            var queue = new Queue<Point>();
            queue.Enqueue(seed);
            int placed = 0;
            int guard = 0;

            while (queue.Count > 0 && placed < patchGoal && guard++ < 8000)
            {
                var p = queue.Dequeue();
                if (IsBlockedForScatter(p, spawnTiles, avoidRadius, waterCells, cliffCells))
                {
                    continue;
                }
                if (!map.Metrics.GetCell(p, out int cell))
                {
                    continue;
                }
                if (map.Technos[cell] != null)
                {
                    continue;
                }

                var treeType = treeTypes[random.Next(treeTypes.Count)];
                if (!map.Technos.Add(cell, new Terrain
                {
                    Type = treeType,
                    Icon = treeType.IsTransformable ? random.Next(0, 24) : 0,
                    Trigger = Trigger.None
                }))
                {
                    continue;
                }
                placed++;

                foreach (var n in Neighbors8(p))
                {
                    if (!map.Bounds.Contains(n))
                    {
                        continue;
                    }
                    if (random.NextDouble() < 0.68)
                    {
                        queue.Enqueue(n);
                    }
                }
            }
        }

        private static bool TrySetTemplate(Map map, Point p, TemplateType type, int icon)
        {
            if (!map.Metrics.GetCell(p, out _))
            {
                return false;
            }
            map.Templates[p] = new Template { Type = type, Icon = icon };
            return true;
        }

        private static bool IsBlockedForScatter(
            Point p,
            IReadOnlyList<Point> spawnTiles,
            int avoidRadius,
            HashSet<Point> waterCells,
            HashSet<Point> cliffCells)
        {
            if (IsNearAnySpawn(p, spawnTiles, avoidRadius))
            {
                return true;
            }
            if (waterCells.Contains(p))
            {
                return true;
            }
            if (cliffCells.Contains(p))
            {
                return true;
            }
            return false;
        }

        private static int MinDistanceToSet(Point p, HashSet<Point> cells)
        {
            int min = int.MaxValue;
            foreach (var c in cells)
            {
                int d = Math.Abs(p.X - c.X) + Math.Abs(p.Y - c.Y);
                if (d < min)
                {
                    min = d;
                }
            }
            return min == int.MaxValue ? 999 : min;
        }

        private static bool IsNearAnySpawn(Point p, IReadOnlyList<Point> spawnTiles, int radius)
        {
            foreach (var s in spawnTiles)
            {
                if (IsNear(s, p, radius))
                {
                    return true;
                }
            }
            return false;
        }

        private static bool IsNear(Point a, Point b, int radius)
        {
            return Math.Abs(a.X - b.X) <= radius && Math.Abs(a.Y - b.Y) <= radius;
        }

        private static IEnumerable<Point> Neighbors8(Point p)
        {
            yield return new Point(p.X - 1, p.Y);
            yield return new Point(p.X + 1, p.Y);
            yield return new Point(p.X, p.Y - 1);
            yield return new Point(p.X, p.Y + 1);
            yield return new Point(p.X - 1, p.Y - 1);
            yield return new Point(p.X + 1, p.Y - 1);
            yield return new Point(p.X - 1, p.Y + 1);
            yield return new Point(p.X + 1, p.Y + 1);
        }

        private static IEnumerable<Point> EnumerateRect(Rectangle rect)
        {
            for (int y = rect.Top; y < rect.Bottom; y++)
            {
                for (int x = rect.Left; x < rect.Right; x++)
                {
                    yield return new Point(x, y);
                }
            }
        }
    }
}